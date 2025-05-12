FROM node:24-alpine3.38
COPY . .
RUN npm ci
EXPOSE 3200
VOLUME ["/cache"]
CMD ["node", "/index.js"]
