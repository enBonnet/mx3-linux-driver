#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

usage() {
    cat <<EOF
Usage: $(basename "$0") <target> [target ...]

Build targets:
  ppa      Build Debian source package for Ubuntu PPA (Launchpad)
  aur      Build Arch Linux package for AUR
  copr     Build SRPM for Fedora COPR
  obs      Prepare package for openSUSE OBS
  all      Build all targets

Deploy targets:
  ppa:deploy   Build and upload to PPA
  aur:deploy   Build and publish to AUR
  copr:deploy  Build and upload SRPM to COPR
  obs:deploy   Build and upload to OBS
  deploy       Deploy all targets

Other:
  clean        Remove build artifacts
  -h/--help    Show this help

Environment variables:
  PPA:   GPG_DIR   - GPG keyring dir (default: ~/.gnupg)
         DPUT_CF   - Custom dput.cf path (optional)
         GPG_KEY   - GPG key ID for signing (optional)
  AUR:   SSH_DIR   - SSH dir with AUR key (default: ~/.ssh)
  COPR:  COPR_CONFIG - copr-cli config dir (default: ~/.config/copr)
  OBS:   OSC_RC    - osc config file (default: ~/.oscrc)
EOF
    exit 1
}

build_ppa() {
    echo ">>> Building PPA package..."
    bash "$SCRIPT_DIR/ppa/build.sh"
}

build_aur() {
    echo ">>> Building AUR package..."
    bash "$SCRIPT_DIR/aur/build.sh"
}

build_copr() {
    echo ">>> Building COPR package..."
    bash "$SCRIPT_DIR/copr/build.sh"
}

build_obs() {
    echo ">>> Building OBS package..."
    bash "$SCRIPT_DIR/obs/build.sh"
}

deploy_ppa() {
    echo ">>> Deploying to PPA..."
    bash "$SCRIPT_DIR/ppa/deploy.sh"
}

deploy_aur() {
    echo ">>> Deploying to AUR..."
    bash "$SCRIPT_DIR/aur/deploy.sh"
}

deploy_copr() {
    echo ">>> Deploying to COPR..."
    bash "$SCRIPT_DIR/copr/deploy.sh"
}

deploy_obs() {
    echo ">>> Deploying to OBS..."
    bash "$SCRIPT_DIR/obs/deploy.sh"
}

clean() {
    echo ">>> Cleaning build artifacts..."
    rm -rf "$SCRIPT_DIR/aur/pkg" "$SCRIPT_DIR/copr/output" "$SCRIPT_DIR/obs/output"
    make -C "$PROJECT_DIR" clean 2>/dev/null || true
    rm -rf "$PROJECT_DIR/dist"
    echo "Done."
}

TARGETS=()

if [ $# -eq 0 ]; then
    usage
fi

while [ $# -gt 0 ]; do
    case "$1" in
        ppa|aur|copr|obs)     TARGETS+=("build_$1") ;;
        ppa:deploy)            TARGETS+=("deploy_ppa") ;;
        aur:deploy)            TARGETS+=("deploy_aur") ;;
        copr:deploy)           TARGETS+=("deploy_copr") ;;
        obs:deploy)            TARGETS+=("deploy_obs") ;;
        deploy)                TARGETS+=(deploy_ppa deploy_aur deploy_copr deploy_obs) ;;
        all)                   TARGETS+=(build_ppa build_aur build_copr build_obs) ;;
        clean)                 clean; exit 0 ;;
        -h|--help)             usage ;;
        *) echo "Unknown target: $1" >&2; usage ;;
    esac
    shift
done

for target in "${TARGETS[@]}"; do
    "$target"
done