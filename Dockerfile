# syntax=docker/dockerfile:1
FROM ubuntu:bionic
ENV TERM linux
ENV DEBIAN_FRONTEND noninteractive
ENV TZ Etc/UTC
WORKDIR /malloc
COPY ./malloc /malloc
RUN apt update
RUN apt install make
RUN apt install gcc-6 g++-6 -y
RUN gcc-6 -v
RUN make
CMD ./malloc