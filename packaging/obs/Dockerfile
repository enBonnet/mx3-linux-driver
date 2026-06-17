FROM opensuse/tumbleweed

RUN zypper --non-interactive install --no-recommends \
        osc \
        build \
        rpm-build \
        systemd-rpm-macros \
        git \
        make \
        gcc \
        && zypper clean -a

WORKDIR /src

ENTRYPOINT ["bash"]