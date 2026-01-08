ARG VARIANT=debian13
FROM mcr.microsoft.com/devcontainers/cpp:${VARIANT}

ARG TYPST=0.14.2
ENV TYPST_VERSION=${TYPST}

# Install needed packages. Use a separate RUN statement to add your own dependencies.
RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install --no-install-recommends gcc build-essential openmpi-bin libopenmpi-dev ca-certificates curl xz-utils fontconfig fonts-texgyre

# Enable contrib repo (for fonts only - shows at duplicated)
RUN echo "deb http://deb.debian.org/debian trixie main contrib" > /etc/apt/sources.list.d/contrib.list \
    && apt-get update \
    && apt-get install --no-install-recommends -y fonts-ibm-plex \
    && rm /etc/apt/sources.list.d/contrib.list \
    && apt-get update

RUN echo 'unset PS1' >> /home/vscode/.zshrc && \
    echo '# Ensure prompt is set by Oh My Zsh, not container default' >> /home/vscode/.zshrc && \
    chown vscode:vscode /home/vscode/.zshrc

RUN curl -fsSL \
        https://github.com/typst/typst/releases/download/v${TYPST_VERSION}/typst-x86_64-unknown-linux-musl.tar.xz \
    | tar -xJ \
    && mv typst-x86_64-unknown-linux-musl/typst /usr/local/bin/typst \
    && rm -rf typst-x86_64-unknown-linux-musl
