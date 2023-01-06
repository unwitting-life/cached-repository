REM https://www.ibm.com/docs/en/arl/9.7?topic=certification-extracting-certificate-keys-from-pfx-file
openssl.exe pkcs12 -in localhost.pfx -nocerts -out localhost.key
openssl.exe pkcs12 -in localhost.pfx -clcerts -nokeys -out localhost.crt
openssl.exe rsa -in localhost.key -out localhost_decrypted.key
openssl.exe rsa -in localhost.key -outform PEM -out localhost_encrypted_pem.key
