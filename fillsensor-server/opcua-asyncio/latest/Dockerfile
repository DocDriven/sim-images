FROM debian:bookworm-slim AS base

# set environment variables here with 'ENV VAR=value'
# this allows dynamic customization of container behavior

# address of the PLC OPC UA server
# ENV OPCUA_SERVER_ADDRESS=

# update index and install packages if necessary with
# configure the main environment for build and release here
ARG YOUR_ENV
ENV YOUR_ENV=${YOUR_ENV} \
  PYTHONFAULTHANDLER=1 \
  PYTHONUNBUFFERED=1 \
  PYTHONHASHSEED=random \
  PIP_NO_CACHE_DIR=off \
  PIP_DISABLE_PIP_VERSION_CHECK=on \
  PIP_DEFAULT_TIMEOUT=100 \
  SHELL="/bin/bash" \
  # Poetry's configuration:
  POETRY_NO_INTERACTION=1 \
  POETRY_VIRTUALENVS_CREATE=true \
  POETRY_CACHE_DIR='/opt/cache/pypoetry' \
  POETRY_HOME='/opt/poetry' \
  POETRY_VERSION=1.8.3

# venv is a needed dependency for poetry and the installer to resove dependencies outside of poetry
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    python3.11 \
    libssl-dev \
    libsqlite3-dev \
    python3-venv 

FROM base AS builder

# optionally include the lock file if existing
COPY pyproject.toml poetry.loc[k] .

# keyrings and pkginfo need to be updated to get the latest resources off pip. 
# this is a known issue with some pip versions falling back to older packages
RUN mkdir /opt/poetry && \ 
          python3 -m venv $POETRY_HOME && \
          $POETRY_HOME/bin/pip install poetry==1.8.3 && \
          $POETRY_HOME/bin/poetry config keyring.enabled false && \
          $POETRY_HOME/bin/poetry self add keyrings.cryptfile@latest && \
          $POETRY_HOME/bin/poetry self add pkginfo==1.10.0 && \
          $POETRY_HOME/bin/poetry --version && \
          $POETRY_HOME/bin/poetry install

FROM base AS runtime

COPY --from=builder /opt /opt
ARG MODE=755
COPY --chmod=$MODE ./app/server.py ./app/WS.NodeSet2.xml /app/

# COPY ./pki /pki

# startup is controlled by this script which depends on environment variables
ARG MODE=755
COPY --chmod=$MODE startup.sh pyproject.toml /

# VOLUME /database

# will be executed on startup
ENTRYPOINT [ "usr/bin/env" ]

# arguments passed to entrypoint, ensures that environment variables are set
# 
# CMD [ "/bin/bash", "/startup.sh" ]
CMD ["/bin/bash", "-c", "$POETRY_HOME/bin/poetry run ./startup.sh"]