## Test users:
username: `test` password: `qwe123`

username: `test2` password: `qwe123`

## Creating certificates:
```bash
openssl genrsa -out server.key 3072
openssl req -new -x509 -key server.key -sha256 -out server.pem -days 730
```

## cURL:
Supress self-signed certificate warning with -k
