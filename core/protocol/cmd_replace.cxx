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

#include "cmd_replace.hxx"

#include "core/utils/byteswap.hxx"
#include "core/utils/unsigned_leb128.hxx"
#include "frame_info_utils.hxx"

#include <cstring>
#include <gsl/assert>

namespace couchbase::core::protocol
{
bool
replace_response_body::parse(key_value_status_code status,
                             const header_buffer& header,
                             std::uint8_t framing_extras_size,
                             std::uint16_t /* key_size */,
                             std::uint8_t extras_size,
                             const std::vector<std::byte>& body,
                             const cmd_info& /* info */)
{
    Expects(header[1] == static_cast<std::byte>(opcode));
    if (status == key_value_status_code::success) {
        std::vector<std::uint8_t>::difference_type offset = framing_extras_size;
        if (extras_size == 16) {
            memcpy(&token_.partition_uuid, body.data() + offset, sizeof(token_.partition_uuid));
            token_.partition_uuid = utils::byte_swap(token_.partition_uuid);
            offset += 8;

            memcpy(&token_.sequence_number, body.data() + offset, sizeof(token_.sequence_number));
            token_.sequence_number = utils::byte_swap(token_.sequence_number);
            return true;
        }
    }
    return false;
}

void
replace_request_body::id(const document_id& id)
{
    key_ = make_protocol_key(id);
}

void
replace_request_body::durability(protocol::durability_level level, std::optional<std::uint16_t> timeout)
{
    if (level == protocol::durability_level::none) {
        return;
    }

    add_durability_frame_info(framing_extras_, level, timeout);
}

void
replace_request_body::preserve_expiry()
{
    add_preserve_expiry_frame_info(framing_extras_);
}

void
replace_request_body::fill_extras()
{
    extras_.resize(sizeof(flags_) + sizeof(expiry_));

    std::uint32_t field = utils::byte_swap(flags_);
    memcpy(extras_.data(), &field, sizeof(field));

    field = utils::byte_swap(expiry_);
    memcpy(extras_.data() + sizeof(flags_), &field, sizeof(field));
}
} // namespace couchbase::core::protocol