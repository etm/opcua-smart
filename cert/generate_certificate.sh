#!/bin/sh
cd "$(dirname "$0")"
openssl req -x509 -newkey rsa:2048 -keyout cert_key.pem -out cert.pem -days 355 -nodes -reqexts v3_req -extensions v3_req -subj "/CN=ruby-opcua" -config generate_certificate.cnf
openssl x509 -outform der -in cert.pem -out cert.der
openssl rsa -inform PEM -in cert_key.pem -outform DER -out cert_key.der
xxd -i cert.der > cert.h
xxd -i cert_key.der > cert_key.h
openssl x509 -inform der -in cert.der -noout -text
