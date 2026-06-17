FROM archlinux:latest

RUN pacman -Syu --noconfirm --needed \
        base-devel \
        git \
        namcap \
        openssh \
        && rm -rf /var/cache/pacman/pkg/*

RUN useradd -m -G wheel builder && \
    echo "builder ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers

USER builder
WORKDIR /home/builder

ENTRYPOINT ["bash"]