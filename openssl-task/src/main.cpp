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

using BIO_ptr = std::unique_ptr<BIO, decltype(&BIO_free)>;

// Only open stdin BIO once
static bool
fingerprint_stdin(BIO_ptr &bio)
{
    if (!bio) {
        bio.reset(BIO_new_fp(stdin, 0));
    }
    return do_fingerprint(bio.get(), "-");
}

static bool
fingerprint_file(const char *fname)
{
    BIO_ptr bio {BIO_new_file(fname, "r"), &BIO_free};
    return do_fingerprint(bio.get(), fname);
}

// usage: calcdigest [ CERT1 [CERT2 ...]]
// No CERTs => read one from stdin
// If CERT is ‘-’, read from stdin, Multiple ‘-’ are supported.
int
main(int argc, char **argv)
{
    BIO_ptr stdin_bio {nullptr, &BIO_free};

    if (argc == 1) {
        return fingerprint_stdin(stdin_bio) ? 0 : 1;
    }

    bool all_ok = true;
    for (int argi = 1; argi < argc; ++argi) {
        const char *a = argv[argi];

        bool ok = (!std::strcmp(a, "-"))
            ? fingerprint_stdin(stdin_bio)
            : fingerprint_file(a);

        all_ok = ok && all_ok;
    }

    return all_ok ? 0 : 1;
}
