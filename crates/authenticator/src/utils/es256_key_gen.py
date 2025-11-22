from pathlib import Path

from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives import serialization

def generate_es256_keypair():
    private_key = ec.generate_private_key(ec.SECP256R1())

    private_pem = private_key.private_bytes(
        encoding=serialization.Encoding.PEM,
        format=serialization.PrivateFormat.PKCS8,
        encryption_algorithm=serialization.NoEncryption(),
    )

    public_key = private_key.public_key()
    public_pem = public_key.public_bytes(
        encoding=serialization.Encoding.PEM,
        format=serialization.PublicFormat.SubjectPublicKeyInfo,
    )

    return private_pem, public_pem

def main():
    priv_pem, pub_pem = generate_es256_keypair()

    out_dir = Path(".")
    priv_path = out_dir/"es256-private-key.pem"
    pub_path = out_dir/"es256-public-key.pem"

    priv_path.write_bytes(priv_pem)
    pub_path.write_bytes(pub_pem)

    print(f"Private key written to: {priv_path.resolve()}")
    print(f"Public key written to:  {pub_path.resolve()}")


if __name__ == "__main__":
    main()
