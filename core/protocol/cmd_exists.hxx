/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *   Copyright 2020-2021 Couchbase, Inc.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#pragma once

#include "client_opcode.hxx"
#include "cmd_info.hxx"
#include "core/document_id.hxx"
#include "core/io/mcbp_message.hxx"
#include "status.hxx"
#include <couchbase/cas.hxx>

namespace couchbase::core::protocol
{

class exists_response_body
{
  public:
    static const inline client_opcode opcode = client_opcode::observe;

  private:
    std::uint16_t partition_id_;
    std::vector<std::byte> key_;
    std::uint8_t status_;
    std::uint64_t cas_;

  public:
    [[nodiscard]] std::uint16_t partition_id() const
    {
        return partition_id_;
    }

    [[nodiscard]] couchbase::cas cas() const
    {
        return couchbase::cas{ cas_ };
    }

    [[nodiscard]] const auto& key() const
    {
        return key_;
    }

    [[nodiscard]] std::uint8_t status() const
    {
        return status_;
    }

    bool parse(key_value_status_code status,
               const header_buffer& header,
               std::uint8_t framing_extras_size,
               std::uint16_t key_size,
               std::uint8_t extras_size,
               const std::vector<std::byte>& body,
               const cmd_info& info);
};

class exists_request_body
{
  public:
    using response_body_type = exists_response_body;
    static const inline client_opcode opcode = client_opcode::observe;

  private:
    std::uint16_t partition_id_;
    std::vector<std::byte> key_;
    std::vector<std::byte> value_{};

  public:
    void id(std::uint16_t partition_id, const document_id& id);

    [[nodiscard]] const std::string& key() const
    {
        /* for observe key goes in the body */
        return empty_string;
    }

    [[nodiscard]] const auto& framing_extras() const
    {
        return empty_buffer;
    }

    [[nodiscard]] const auto& extras() const
    {
        return empty_buffer;
    }

    [[nodiscard]] const auto& value()
    {
        if (value_.empty()) {
            fill_body();
        }
        return value_;
    }

    [[nodiscard]] std::size_t size()
    {
        if (value_.empty()) {
            fill_body();
        }
        return value_.size();
    }

  private:
    void fill_body();
};

} // namespace couchbase::core::protocol