
FROM debian:bookworm-slim

ARG USER_UID=1000
ARG USER_GID=1000

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
      build-essential \
      bison \
      flex \
      libgmp-dev \
      libmpc-dev \
      libmpfr-dev \
      libisl-dev \
      texinfo \
      wget \
      ca-certificates \
      xorriso \
      grub-pc-bin \
      grub-common \
      mtools \
      nasm \
      file \
      git \
      make \
      sudo \
      bsdmainutils \
   && rm -rf /var/lib/apt/lists/*

RUN groupadd --gid ${USER_GID} dev \
   && useradd --uid ${USER_UID} --gid ${USER_GID} --create-home --shell /bin/bash dev \
   && echo 'dev ALL=(ALL) NOPASSWD:ALL' > /etc/sudoers.d/dev

USER dev
WORKDIR /workspace

CMD ["/bin/bash"]