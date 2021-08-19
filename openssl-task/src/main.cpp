#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cinttypes>
#include <iostream>
#include <memory>

#include <openssl/pem.h>
#include <openssl/x509.h>

static bool
do_fingerprint(BIO *b, const char *label)
{
    std::unique_ptr<X509, decltype(&X509_free)> cert {
        PEM_read_bio_X509(b, nullptr, nullptr, nullptr),
        &X509_free,
    };

    if (!cert) {
        std::cerr << label << ": Failed to read cert\n";
        return false;
    }

    const EVP_MD *alg = EVP_sha256();
    int size = EVP_MD_size(alg);

    if (size <= 0 || EVP_MAX_MD_SIZE < size) {
        throw std::runtime_error{"Invalid digest size"};
    }

    uint8_t buf[size];          // size is bounded

    unsigned dlen = 0;
    if (!X509_digest(cert.get(), alg, buf, &dlen)) {
        std::cerr << label << ": Failed to calculate digest\n";
        return false;
    }

    if (dlen != static_cast<unsigned>(size)) {
        throw std::runtime_error{"Unexpected digest size"};
    }

    std::cout << label << ": ";
    char hexbuf[3];
    for (unsigned i = 0; i < dlen; ++i) {
        std::sprintf(hexbuf, "%02" PRIx8, buf[i]);
        std::cout << hexbuf;
    }
    std::cout << std::endl;

    return true;
}

// usage: calcdigest [ CERT1 [CERT2 ...]]
// No CERTs => read one from stdin
// If CERT is ‘-’, read from stdin, Multiple ‘-’ are supported.
int
main(int argc, char **argv)
{
    using BIO_ptr = std::unique_ptr<BIO, decltype(&BIO_free)>;

    if (argc == 1) {
        BIO_ptr bio {BIO_new_fp(stdin, 0), &BIO_free};
        return do_fingerprint(bio.get(), "-") ? 0 : 1;
    }

    BIO_ptr stdin_bio {nullptr, &BIO_free};

    bool all_ok = true;
    for (int argi = 1; argi < argc; ++argi) {
        const char *a = argv[argi];

        bool ok;
        if (!std::strcmp(a, "-")) {
            if (!stdin_bio) {
                stdin_bio.reset(BIO_new_fp(stdin, 0));
            }
            ok = do_fingerprint(stdin_bio.get(), a);
        } else {
            BIO_ptr bio {BIO_new_file(a, "r"), &BIO_free};
            ok = do_fingerprint(bio.get(), a);
        }

        all_ok = ok && all_ok;
    }

    return all_ok ? 0 : 1;
}
