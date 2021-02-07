FROM alpine:3.12

RUN apk add --no-cache \
	make \
	gcc \
	g++ \
	cmake \
	git \
	python3-dev \
	sfml-dev \
	libxml2-dev \
	libxslt-dev \
	py3-pip && \
	pip install gcovr

COPY ./entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

WORKDIR /litecgss

CMD [ "/entrypoint.sh" ]