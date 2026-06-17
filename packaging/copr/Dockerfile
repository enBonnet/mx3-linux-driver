FROM fedora:latest

RUN dnf install -y --setopt=install_weak_deps=False \
        rpm-build \
        rpmdevtools \
        copr-cli \
        git \
        make \
        gcc \
        systemd-rpm-macros \
        && dnf clean all

WORKDIR /src

ENTRYPOINT ["bash"]