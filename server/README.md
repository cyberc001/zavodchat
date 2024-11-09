## Test user:
username: `test`
password: `qwe123`

## Creating certificates:
  $ openssl genrsa -des3 -passout pass:x -out server.pass.key 2048
  $ openssl rsa -passin pass:x -in server.pass.key -out server.key
  $ openssl req -new -key server.key -out server.csr
  $ openssl x509 -req -days 365 -in server.csr -signkey server.key -out server.crt

## cURL:
Supress self-signed certificate warning with -k
