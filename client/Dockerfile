FROM node:25

# install emscripten
RUN mkdir /3rdparty
RUN git clone https://github.com/emscripten-core/emsdk.git /3rdparty/emsdk; cd /3rdparty/emsdk;\
	./emsdk install 4.0.23; ./emsdk activate 4.0.23
ENV PATH="$PATH:/3rdparty/emsdk:/3rdparty/emsdk/upstream/emscripten"

# install node_modules
COPY package.json package-lock.json svelte.config.js vite.config.js jsconfig.json .npmrc .env download_model.sh.patch client.key client.pem /client/
WORKDIR /client
RUN npm install

# compile rnnoise
RUN git clone https://github.com/xiph/rnnoise.git /3rdparty/rnnoise; cd /3rdparty/rnnoise;\
	cp /client/download_model.sh.patch .; patch < download_model.sh.patch;\
	./autogen.sh; emconfigure ./configure; emmake make

# copy source code
COPY static/ /client/static/
COPY src/ /client/src/

# compile webasm
WORKDIR /client/src
RUN make -j 4


#ENTRYPOINT ["tail", "-f", "/dev/null"]
ENV HOST=0.0.0.0
ENTRYPOINT ["npm", "run", "dev", "--", "--host", "0.0.0.0"]