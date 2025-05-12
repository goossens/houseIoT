FROM node:24-alpine
COPY . .
RUN npm ci
EXPOSE 8010
VOLUME ["/cache"]
CMD ["node", "/index.js"]
