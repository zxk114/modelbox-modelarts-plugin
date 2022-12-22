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

#ifndef MODELARTS_CIPHER_H_
#define MODELARTS_CIPHER_H_

#include <config.h>
#include <openssl/rsa.h>
#include <status.h>
#include <stdio.h>
#include <unistd.h>

#include <memory>
#include <string>

namespace modelarts {

class Cipher {
 public:
  Cipher() = default;
  virtual ~Cipher() = default;

  modelbox::Status Init(const std::string &key_path, bool isPrivateKey);
  modelbox::Status DecryptFromBase64(const std::string &cipher,
                                     std::string &plain);
  modelbox::Status DecryptMsg(const std::string &cipher, std::string &plain);

 private:
  modelbox::Status CipherMsg(const std::string &input,
                             std::shared_ptr<char> output, size_t &outputLen,
                             bool isDecrypt);

  modelbox::Status CipherMsg(const std::shared_ptr<EVP_PKEY_CTX> &context,
                             const unsigned char *input, size_t inLen,
                             unsigned char *output, size_t &outLen,
                             bool isDecrypt);
  modelbox::Status InitKeyByPath(const std::string &keyPath, bool isPrivateKey);
  modelbox::Status InitKey(const std::shared_ptr<char> key, bool isPrivateKey,
                           size_t key_length);
  modelbox::Status GetCipherSizeInfo(bool is_decrypt, int input_size,
                                     int &rsa_size, int &in_block_size,
                                     int &out_buffer_size);
  modelbox::Status GetContextAndKey(std::shared_ptr<EVP_PKEY_CTX> &context,
                                    std::shared_ptr<EVP_PKEY> &key,
                                    bool isDecrypt);
  bool isPrivateKey_{false};
  std::shared_ptr<RSA> rsa_{nullptr};
};

}  // namespace modelarts

#endif  // MODELARTS_CHIPER_H_