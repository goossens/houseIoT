# see https://github.com/lipanski/docker-static-website

FROM lipanski/docker-static-website:latest
COPY . .
EXPOSE 80
CMD ["/busybox", "httpd", "-f", "-v"]
