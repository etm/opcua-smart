#!/usr/bin/bash
#openssl req -x509 -newkey rsa:2048 -keyout cert_key.pem -out cert.pem -days 355 -nodes -reqexts v3_req -extensions v3_req -subj "/CN=opc2mqtt" -config generate_certificate.cnf
#openssl x509 -outform der -in cert.pem -out cert.der
#openssl rsa -inform PEM -in cert_key.pem -outform DER -out cert_key.der
openssl x509 -inform der -in cert.der -noout -text
