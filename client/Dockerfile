FROM node:25

COPY package.json package-lock.json svelte.config.js vite.config.js jsconfig.json .npmrc .env /client/
WORKDIR /client
RUN npm install
COPY static/ /client/static/
COPY src/ /client/src/

#ENTRYPOINT ["tail", "-f", "/dev/null"]
ENTRYPOINT ["npm", "run", "dev"]
