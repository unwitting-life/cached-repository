REM https://www.ibm.com/docs/en/arl/9.7?topic=certification-extracting-certificate-keys-from-pfx-file
openssl pkcs12 -in localhost.pfx -nocerts -out localhost.key
openssl pkcs12 -in localhost.pfx -clcerts -nokeys -out localhost.crt
openssl rsa -in localhost.key -out localhost_decrypted.key
REM openssl rsa -in localhost.key -outform PEM -out localhost_encrypted_pem.key
