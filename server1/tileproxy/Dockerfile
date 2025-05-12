FROM node:24-alpine
COPY . .
RUN npm ci
EXPOSE 3200
VOLUME ["/cache"]
CMD ["node", "/index.js"]
