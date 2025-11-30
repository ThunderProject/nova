#!/usr/bin/env python3
import base64
import os
import secrets
import stat
from datetime import datetime, timedelta, timezone

from cryptography import x509
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives.asymmetric.ec import EllipticCurvePrivateKey, EllipticCurvePublicKey
from cryptography.x509 import Name
from cryptography.x509.oid import NameOID, ExtendedKeyUsageOID


def write(path: str, data: bytes):
    with open(path, "wb") as writer:
        writer.write(data)
    os.chmod(path, stat.S_IRUSR | stat.S_IWUSR)


def main():
    CN: str = "nova authenticator"
    VALIDITY_DAYS: int = 3652
    SAN_HOSTS: list[str] = ["127.0.0.1", "localhost"]

    KEY_OUT: str = "server.key.pem"
    CERT_OUT: str = "server.cert.pem"
    PW_OUT: str = "server.key.password.txt"

    password: bytes = base64.urlsafe_b64encode(secrets.token_bytes(32))
    write(PW_OUT, password)

    private_key: EllipticCurvePrivateKey = ec.generate_private_key(ec.SECP384R1())
    public_key: EllipticCurvePublicKey = private_key.public_key()
    subject: Name = x509.Name([x509.NameAttribute(NameOID.COMMON_NAME, CN)])
    san_list = []

    import ipaddress
    for host in SAN_HOSTS:
        try:
            san_list.append(x509.IPAddress(ipaddress.ip_address(host)))
        except ValueError:
            san_list.append(x509.DNSName(host))

    san = x509.SubjectAlternativeName(san_list)

    not_before = datetime.now(timezone.utc) - timedelta(minutes=5)
    not_after  = not_before + timedelta(days=VALIDITY_DAYS)

    builder = (
        x509.CertificateBuilder()
        .subject_name(subject)
        .issuer_name(subject)
        .public_key(public_key)
        .serial_number(x509.random_serial_number())
        .not_valid_before(not_before)
        .not_valid_after(not_after)
        .add_extension(san, critical=False)
        .add_extension(x509.BasicConstraints(ca=False, path_length=None), critical=True)
        .add_extension(x509.KeyUsage(
            digital_signature=True,
            content_commitment=False,
            key_encipherment=False,
            data_encipherment=False,
            key_agreement=False,
            key_cert_sign=False,
            crl_sign=False,
            encipher_only=False,
            decipher_only=False,
        ), critical=True)
        .add_extension(x509.ExtendedKeyUsage([
            ExtendedKeyUsageOID.SERVER_AUTH,
            ExtendedKeyUsageOID.CLIENT_AUTH,
        ]), critical=False)
    )

    certificate = builder.sign(
        private_key=private_key,
        algorithm=hashes.SHA384()
    )

    key_pem = private_key.private_bytes(
        serialization.Encoding.PEM,
        serialization.PrivateFormat.PKCS8,
        serialization.BestAvailableEncryption(password),
    )
    cert_pem = certificate.public_bytes(serialization.Encoding.PEM)

    write(KEY_OUT, key_pem)
    write(CERT_OUT, cert_pem)

    print("\nGenerated:")
    print(f"  {KEY_OUT}")
    print(f"  {CERT_OUT}")
    print(f"  {PW_OUT}")
    print("\nVerification:")
    print(f"  openssl x509 -in {CERT_OUT} -noout -text | less")


if __name__ == "__main__":
    main()
