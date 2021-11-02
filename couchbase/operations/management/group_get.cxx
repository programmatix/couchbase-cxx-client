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

#include <couchbase/operations/management/group_get.hxx>

#include <couchbase/errors.hxx>

#include <couchbase/utils/json.hxx>
#include <couchbase/operations/management/rbac_json.hxx>

namespace couchbase::operations::management
{
std::error_code
group_get_request::encode_to(encoded_request_type& encoded, http_context& /* context */) const
{
    encoded.method = "GET";
    encoded.path = fmt::format("/settings/rbac/groups/{}", name);
    encoded.headers["content-type"] = "application/x-www-form-urlencoded";
    return {};
}

group_get_response
group_get_request::make_response(error_context::http&& ctx, const encoded_response_type& encoded) const
{
    group_get_response response{ std::move(ctx) };
    if (!response.ctx.ec) {
        switch (encoded.status_code) {
            case 200: {
                try {
                    response.group = utils::json::parse(encoded.body).as<rbac::group>();
                } catch (const tao::pegtl::parse_error&) {
                    response.ctx.ec = error::common_errc::parsing_failure;
                    return response;
                }
            } break;
            case 404:
                response.ctx.ec = error::management_errc::group_not_found;
                break;
            default:
                response.ctx.ec = error::common_errc::internal_server_failure;
                break;
        }
    }
    return response;
}
} // namespace couchbase::operations::management