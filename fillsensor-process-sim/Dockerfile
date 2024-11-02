FROM debian:bookworm-slim AS base

# set environment variables here with 'ENV VAR=value'
# this allows dynamic customization of container behavior


# update index and install packages if necessary with
RUN apt-get update && apt-get install -y \
    python3.11 \
    libssl-dev

FROM base AS builder

RUN apt-get install -y \
    curl \
    python3-pip \
    libffi-dev \
    pkg-config

RUN curl -L sh.rustup.rs | sh -s -- -y

ENV PATH="/root/.cargo/bin:${PATH}"

RUN pip install asyncua --break-system-packages

FROM base AS runtime

COPY --from=builder /usr/local/lib/python3.11/dist-packages /usr/local/lib/python3.11/dist-packages

COPY /app /usr/src/app

ENTRYPOINT [ "python3.11", "/usr/src/app/sim.py"]
