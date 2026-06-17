#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
VERSION="$(tr -d '\n' < "$PROJECT_DIR/VERSION")"

SSH_DIR="${SSH_DIR:-$HOME/.ssh}"
AUR_REPO="${AUR_REPO:-ssh://aur@aur.archlinux.org/mx3.git}"
SSH_KEY="${SSH_KEY:-}"

echo "=== Deploying mx3 $VERSION to AUR ==="

bash "$SCRIPT_DIR/build.sh"

echo "Publishing PKGBUILD and .SRCINFO to AUR..."

SSH_MOUNT="$(mktemp -d ${TMPDIR:-/tmp}/aur_ssh.XXXXXX)"

if [ -n "$SSH_KEY" ] && [ -f "$SSH_KEY" ]; then
    KEYNAME="$(basename "$SSH_KEY")"
    cp "$SSH_KEY" "$SSH_MOUNT/$KEYNAME"
    cp "${SSH_KEY}.pub" "$SSH_MOUNT/${KEYNAME}.pub" 2>/dev/null || true
    chmod 600 "$SSH_MOUNT/$KEYNAME"
elif [ -d "$SSH_DIR" ]; then
    cp -a "$SSH_DIR"/* "$SSH_MOUNT/" 2>/dev/null || true
    KEYNAME="$(ls "$SSH_MOUNT"/id_* 2>/dev/null | grep -v '\.pub$' | head -1 | xargs basename 2>/dev/null || true)"
fi

KEYNAME="${KEYNAME:-id_ed25519}"

cat > "$SSH_MOUNT/config" << SSHCONFIG
Host aur.archlinux.org
    User aur
    IdentityFile ~/.ssh/$KEYNAME
    StrictHostKeyChecking accept-new
SSHCONFIG
chmod 644 "$SSH_MOUNT/config"

podman run --rm --entrypoint "" \
    -v "$PROJECT_DIR:/src:Z" \
    -v "$SSH_MOUNT:/home/builder/.ssh:Z" \
    -e GIT_AUTHOR_NAME="${GIT_AUTHOR_NAME:-Ender Bonnet}" \
    -e GIT_AUTHOR_EMAIL="${GIT_AUTHOR_EMAIL:-enbonnet@gmail.com}" \
    -e GIT_COMMITTER_NAME="${GIT_COMMITTER_NAME:-Ender Bonnet}" \
    -e GIT_COMMITTER_EMAIL="${GIT_COMMITTER_EMAIL:-enbonnet@gmail.com}" \
    mx3-aur \
    /bin/sh -c "
        set -euo pipefail
        export GIT_AUTHOR_NAME GIT_AUTHOR_EMAIL GIT_COMMITTER_NAME GIT_COMMITTER_EMAIL
        sudo chown -R builder:builder /home/builder/.ssh
        chmod 700 /home/builder/.ssh
        chmod 600 /home/builder/.ssh/id_* 2>/dev/null || true
        ssh-keyscan aur.archlinux.org >> /home/builder/.ssh/known_hosts
        TMPDIR=\$(mktemp -d)
        git clone '$AUR_REPO' \"\$TMPDIR/mx3\"
        cp /src/PKGBUILD /src/.SRCINFO \"\$TMPDIR/mx3/\"
        cd \"\$TMPDIR/mx3\"
        git add PKGBUILD .SRCINFO
        git diff --cached --quiet && echo 'No changes to push' && exit 0
        git commit -m 'Update to $VERSION'
        git push
        echo 'AUR push complete'
    "

rm -rf "$SSH_MOUNT"

echo "=== AUR deploy complete ==="