FROM alpine:3.10

COPY . /mve

RUN apk add --no-cache \
    make \
    g++ \
    jpeg-dev \
    libpng-dev \
    tiff-dev \
    mesa-dev \
    libexecinfo-dev

RUN echo $(ls /usr/include) \
    && cd /mve \
    && make all
    
