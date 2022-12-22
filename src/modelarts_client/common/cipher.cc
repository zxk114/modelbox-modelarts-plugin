/*
 * Copyright 2022 The Modelbox Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cipher.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <securec.h>

#include <algorithm>
#include <fstream>
#include <memory>

#include "modelbox/base/crypto.h"

namespace modelarts {

#define PADDING_DATA_SIZE 66

modelbox::Status Cipher::Init(const std::string &key_path, bool isPrivateKey) {
  auto status = InitKeyByPath(key_path, isPrivateKey);
  if (!status) {
    MBLOG_ERROR << "chiper init failed, error:" << status.WrapErrormsgs();
    return modelbox::STATUS_FAULT;
  }
  return modelbox::STATUS_SUCCESS;
}

static modelbox::Status GetCleanBuff(std::shared_ptr<char> &buf,
                                     size_t length) {
  if (length <= 0) {
    auto msg = std::string("GetCleanBuff, invalid param.") +
               std::string(" length:") + std::to_string(length);
    return {modelbox::STATUS_FAULT, msg};
  }

  auto tmpBuf = (char *)malloc(length);
  if (tmpBuf == nullptr) {
    auto msg = std::string("GetCleanBuff, malloc failed.") +
               std::string(" length:") + std::to_string(length);
    return {modelbox::STATUS_FAULT, msg};
  }

  auto ret = memset_s(tmpBuf, length, 0, length);
  if (ret) {
    MBLOG_ERROR << "GetCleanBuff, memset_s tmpBuf failed.";
    free(tmpBuf);
    return modelbox::STATUS_FAULT;
  }
  buf = std::shared_ptr<char>(tmpBuf, [](char *p) { free(p); });
  return modelbox::STATUS_SUCCESS;
}

static modelbox::Status ReadFile(const std::string &path,
                                 std::shared_ptr<char> &buf, size_t &fileLen) {
  std::ifstream f(path, std::ios::in | std::ios::binary);
  if (!f.is_open()) {
    auto msg = std::string("ReadFile, open failed.") + std::string(" path:") +
               path + std::string(" errorCode: ") + modelbox::StrError(errno);
    return {modelbox::STATUS_FAULT, msg};
  }
  Defer { f.close(); };
  auto begin = f.tellg();
  f.seekg(0, std::ios::end);
  auto end = f.tellg();

  int64_t length = end - begin;
  if (length <= 0) {
    auto msg = std::string("ReadFile, invalid file.");
    return {modelbox::STATUS_FAULT, msg};
  }
  fileLen = (size_t)length;
  f.seekg(0, std::ios::beg);

  if (fileLen >= SIZE_MAX) {
    auto msg =
        std::string("fileLen is invaild.fileLen: ") + std::to_string(fileLen);
    return {modelbox::STATUS_FAULT, msg};
  }

  auto status = GetCleanBuff(buf, fileLen + 1);
  if (!status) {
    status = {status, "ReadFile, GetCleanBuff failed."};
    return status;
  }
  f.read(buf.get(), fileLen);
  buf.get()[fileLen] = '\0';
  return modelbox::STATUS_SUCCESS;
}

modelbox::Status Cipher::InitKey(const std::shared_ptr<char> key,
                                 bool isPrivateKey, size_t key_length) {
  auto keyLen = strnlen(key.get(), key_length);
  if (keyLen <= 0) {
    auto msg = std::string("InitKey, key is empty. keyLen:") +
               std::to_string(keyLen) + std::string(" isPrivateKey:") +
               std::to_string(isPrivateKey);
    MBLOG_WARN << msg;
    return {modelbox::STATUS_FAULT, msg};
  }

  auto bp = BIO_new_mem_buf(key.get(), -1);
  if (bp == nullptr) {
    auto msg = std::string("InitKey, BIO_new_mem_buf failed. isPrivateKey") +
               std::to_string(isPrivateKey);
    MBLOG_WARN << msg;
    return {modelbox::STATUS_FAULT, msg};
  }

  auto tmpFree = std::shared_ptr<BIO>(bp, [](BIO *bp) { BIO_free_all(bp); });

  RSA *rsa = nullptr;
  if (isPrivateKey) {
    rsa = PEM_read_bio_RSAPrivateKey(bp, NULL, NULL, NULL);
  } else {
    rsa = PEM_read_bio_RSA_PUBKEY(bp, NULL, NULL, NULL);
  }

  if (rsa == nullptr) {
    auto msg = std::string("InitKey, read rsa key failed. isPrivateKey") +
               std::to_string(isPrivateKey);
    MBLOG_WARN << msg;
    return {modelbox::STATUS_FAULT, msg};
  }

  isPrivateKey_ = isPrivateKey;
  rsa_ = std::shared_ptr<RSA>(rsa, [](RSA *rsa) { RSA_free(rsa); });

  MBLOG_INFO << "InitKey, load rsa key success. isPrivateKey:" << isPrivateKey;
  return modelbox::STATUS_SUCCESS;
}

modelbox::Status Cipher::InitKeyByPath(const std::string &keyPath,
                                       bool isPrivateKey) {
  if (keyPath.empty()) {
    auto msg = std::string("InitKeyPath, keyPath is empty.") +
               std::string(" isPrivateKey:") + std::to_string(isPrivateKey);
    MBLOG_WARN << msg;
    return {modelbox::STATUS_FAULT, msg};
  }

  std::shared_ptr<char> key;
  size_t fileLen = 0;
  auto status = ReadFile(keyPath, key, fileLen);
  if (!status) {
    status = {status, "InitKeyPath failed. " + std::to_string(isPrivateKey)};
    MBLOG_WARN << status.WrapErrormsgs();
    return status;
  }

  status = InitKey(key, isPrivateKey, fileLen);
  if (!status) {
    status = {status, "InitKeyPath failed. " + std::to_string(isPrivateKey)};
    MBLOG_WARN << status.WrapErrormsgs();
    return status;
  }

  MBLOG_INFO << "InitKeyPath, load key success. isPrivateKey:" << isPrivateKey;
  return modelbox::STATUS_SUCCESS;
}

modelbox::Status Cipher::DecryptFromBase64(const std::string &cipher,
                                           std::string &plain) {
  std::vector<unsigned char> temp_vec;
  auto bool_ret = modelbox::Base64Decode(cipher, &temp_vec);
  std::string temp;
  temp.assign(temp_vec.begin(), temp_vec.end());
  if (bool_ret) {
    MBLOG_ERROR << "Base64Decode failed.";
    return modelbox::STATUS_FAULT;
  }
  auto status = DecryptMsg(cipher, plain);
  if (!status) {
    MBLOG_ERROR << "DecryptMsg failed." << status.WrapErrormsgs();
  }

  return status;
}

modelbox::Status Cipher::GetCipherSizeInfo(bool is_decrypt, int input_size,
                                           int &rsa_size, int &in_block_size,
                                           int &out_buffer_size) {
  rsa_size = RSA_size(rsa_.get());
  in_block_size = rsa_size - PADDING_DATA_SIZE;
  if (is_decrypt) {
    in_block_size = rsa_size;
  }

  if (in_block_size <= 0) {
    return {modelbox::STATUS_FAULT,
            "in_block_size is invaild. " + std::to_string(in_block_size)};
  }

  out_buffer_size =
      ((input_size + in_block_size - 1) / in_block_size) * rsa_size;
  if (in_block_size <= 0 || out_buffer_size < 0) {
    auto msg = "GetCipherSizeInfo failed .in_block_size:" +
               std::to_string(in_block_size) +
               " out_buffer_size:" + std::to_string(out_buffer_size);
    return {modelbox::STATUS_FAULT, msg};
  }
  return modelbox::STATUS_SUCCESS;
}

modelbox::Status Cipher::GetContextAndKey(
    std::shared_ptr<EVP_PKEY_CTX> &context, std::shared_ptr<EVP_PKEY> &key,
    bool isDecrypt) {
  auto privKey = EVP_PKEY_new();
  if (privKey == nullptr) {
    auto msg = "GetContextAndKey, EVP_PKEY_new failed.";
    return {modelbox::STATUS_FAULT, msg};
  }

  key = std::shared_ptr<EVP_PKEY>(privKey,
                                  [](EVP_PKEY *key) { EVP_PKEY_free(key); });
  EVP_PKEY_set1_RSA(privKey, rsa_.get());
  auto ctx = EVP_PKEY_CTX_new(privKey, NULL);
  if (ctx == nullptr) {
    auto msg = "GetContextAndKey, EVP_PKEY_CTX_new failed.";
    return {modelbox::STATUS_FAULT, msg};
  }

  context = std::shared_ptr<EVP_PKEY_CTX>(
      ctx, [](EVP_PKEY_CTX *ctx) { EVP_PKEY_CTX_free(ctx); });

  if (isDecrypt) {
    if (EVP_PKEY_decrypt_init(ctx) <= 0) {
      auto msg = "GetContextAndKey, EVP_PKEY_decrypt_init failed.";
      return {modelbox::STATUS_FAULT, msg};
    }
  } else {
    if (EVP_PKEY_encrypt_init(ctx) <= 0) {
      auto msg = "GetContextAndKey, EVP_PKEY_encrypt_init failed.";
      return {modelbox::STATUS_FAULT, msg};
    }
  }

  EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING);
  EVP_PKEY_CTX_set_rsa_oaep_md(ctx, EVP_sha256());
  EVP_PKEY_CTX_set_rsa_mgf1_md(ctx, EVP_sha256());

  return modelbox::STATUS_SUCCESS;
}

modelbox::Status Cipher::CipherMsg(const std::shared_ptr<EVP_PKEY_CTX> &context,
                                   const unsigned char *input, size_t inLen,
                                   unsigned char *output, size_t &outLen,
                                   bool isDecrypt) {
  int ret = 0;
  if (isDecrypt) {
    ret = EVP_PKEY_decrypt(context.get(), output, &outLen, input, inLen);

  } else {
    ret = EVP_PKEY_encrypt(context.get(), output, &outLen, input, inLen);
  }

  if (ret <= 0 || outLen <= 0) {
    auto msg = "CipherMsg, openssl failed. inLen:" + std::to_string(inLen) +
               " isDecrypt:" + std::to_string(isDecrypt) +
               " ret:" + std::to_string(ret) +
               " outLen:" + std::to_string(outLen);
    return {modelbox::STATUS_FAULT, msg};
  }

  return modelbox::STATUS_SUCCESS;
}

modelbox::Status Cipher::CipherMsg(const std::string &input,
                                   std::shared_ptr<char> output,
                                   size_t &outputLen, bool isDecrypt) {
  if (!isDecrypt && isPrivateKey_) {
    auto msg = "CipherMsg, private ke only support decrypt.";
    return {modelbox::STATUS_FAULT, msg};
  }

  int rsaSize = 0;
  int inBlockSize = 0;
  int outBufSize = 0;
  auto status = GetCipherSizeInfo(isDecrypt, (int)input.size(), rsaSize,
                                  inBlockSize, outBufSize);
  if (!status) {
    status = {status, "CipherMsg, GetCipherSizeInfo failed."};
    return status;
  }

  status = GetCleanBuff(output, outBufSize);
  if (!status) {
    status = {status, "CipherMsg, GetCleanBuff failed."};
    return status;
  }

  std::shared_ptr<EVP_PKEY_CTX> context;
  std::shared_ptr<EVP_PKEY> key;
  status = GetContextAndKey(context, key, isDecrypt);
  if (!status) {
    status = {status, "CipherMsg, GetContextAdnKey failed."};
    return status;
  }

  std::string result;
  size_t offset = 0;
  outputLen = 0;
  while (input.size() > offset) {
    size_t leftSize = input.size() - offset;
    size_t stepSize = std::min(leftSize, (size_t)inBlockSize);

    size_t outLen = (size_t)rsaSize;
    status =
        CipherMsg(context, (unsigned char *)input.c_str() + offset, stepSize,
                  (unsigned char *)output.get() + outputLen, outLen, isDecrypt);
    if (!status) {
      status = {status, "CipherMsg, CipherMsg failed."};
      return status;
    }

    offset += stepSize;
    outputLen += outLen;
  }

  return modelbox::STATUS_SUCCESS;
}

modelbox::Status Cipher::DecryptMsg(const std::string &cipher,
                                    std::string &plain) {
  if (cipher.empty()) {
    auto msg = std::string("DecryptMsg, plain is empty.");
    MBLOG_WARN << msg;
    return {modelbox::STATUS_FAULT, msg};
  }

  std::shared_ptr<char> outBuf;
  size_t outBufLen = 0;
  auto status = CipherMsg(cipher, outBuf, outBufLen, true);
  if (!status) {
    status = {status, "DecryptMsg, CipherMsg failed."};
    MBLOG_WARN << status.WrapErrormsgs();
    return status;
  }

  auto tmp = std::string(outBuf.get(), outBufLen);
  plain = std::move(tmp);
  return modelbox::STATUS_SUCCESS;
}

}  // namespace modelarts
