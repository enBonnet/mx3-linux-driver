FROM debian:trixie-slim

RUN apt-get update -qq && \
    apt-get install -y -qq --no-install-recommends \
        build-essential \
        devscripts \
        debhelper-compat \
        dput-ng \
        python3-distro-info \
        gpg \
        lintian \
        && rm -rf /var/lib/apt/lists/*

WORKDIR /src

ENTRYPOINT ["bash"]