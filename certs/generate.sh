#!/bin/bash
set -euo pipefail

# === Clean up old files ===
rm -f cert_ca.pem key_ca.pem cert{1..3}.pem key{1..3}.pem cert{1..3}.csr \
      cert{1..3}_ext.cnf

# === 1. Generate Root CA ===
echo "Generating Root CA..."
openssl genrsa -out key_ca.pem 4096
openssl req -x509 -new -nodes -key key_ca.pem -sha256 -days 3650 \
    -subj "/C=DE/ST=Hessen/L=Giessen/O=MyOrg/CN=MyRootCA" \
    -out cert_ca.pem

# === IPs for servers ===
IPS=("127.0.0.1" "127.0.0.1" "127.0.0.1")

# === 2. Generate server certs signed by CA ===
for i in 1 2 3; do
    echo "Generating server cert $i..."
    ip="${IPS[$((i-1))]}"

    # Generate server key
    openssl genrsa -out key${i}.pem 2048

    # Generate CSR
    openssl req -new -key key${i}.pem \
        -subj "/C=DE/ST=Hessen/L=Darmstadt/O=TUDa/CN=server${i}" \
        -out cert${i}.csr

    # Create ext config with SAN (Subject Alternative Name)
    cat > cert${i}_ext.cnf <<EOF
subjectAltName = IP:${ip}
basicConstraints = CA:FALSE
keyUsage = digitalSignature, keyEncipherment
extendedKeyUsage = serverAuth
EOF

    # Sign with CA
    openssl x509 -req -in cert${i}.csr \
        -CA cert_ca.pem -CAkey key_ca.pem -CAcreateserial \
        -out cert${i}.pem -days 825 -sha256 \
        -extfile cert${i}_ext.cnf
done

# === 3. Verify certs ===
for i in 1 2 3; do
    echo "Verifying cert${i}.pem..."
    openssl verify -verbose -CAfile cert_ca.pem cert${i}.pem
done

echo "✅ All certificates generated and verified successfully."

