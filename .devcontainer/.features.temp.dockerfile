FROM jb-devcontainer-hpc_prro:latest
COPY --from=jb-devcontainer-features-3d2ae67f96ad7ad62263a0d90be357f5 /tmp/jb-devcontainer-features /tmp/jb-devcontainer-features/
ENV PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/local/vcpkg"
ENV VCPKG_ROOT="/usr/local/vcpkg"
ENV VCPKG_DOWNLOADS="/usr/local/vcpkg-downloads"
ENV TYPST_VERSION="0.14.2"
ENV _CONTAINER_USER="root"
ENV _CONTAINER_USER_HOME="/root"
ENV _REMOTE_USER="vscode"
ENV _REMOTE_USER_HOME="/home/vscode"

USER root
RUN chmod -R 0755 /tmp/jb-devcontainer-features/ghcr.io-devcontainers-features-common-utils-latest \
&& cd /tmp/jb-devcontainer-features/ghcr.io-devcontainers-features-common-utils-latest \
&& chmod +x ./devcontainer-feature-setup.sh \
&& ./devcontainer-feature-setup.sh
USER root
RUN chmod -R 0755 /tmp/jb-devcontainer-features/ghcr.io-devcontainers-features-git-1 \
&& cd /tmp/jb-devcontainer-features/ghcr.io-devcontainers-features-git-1 \
&& chmod +x ./devcontainer-feature-setup.sh \
&& ./devcontainer-feature-setup.sh
USER root
RUN chmod -R 0755 /tmp/jb-devcontainer-features/ghcr.io-devcontainers-extra-features-zsh-plugins-0 \
&& cd /tmp/jb-devcontainer-features/ghcr.io-devcontainers-extra-features-zsh-plugins-0 \
&& chmod +x ./devcontainer-feature-setup.sh \
&& ./devcontainer-feature-setup.sh