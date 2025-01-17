/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *   Copyright 2020-Present Couchbase, Inc.
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

#include <couchbase/codec/tao_json_serializer.hxx>
#include <couchbase/search_meta_data.hxx>

#include "search_row.hxx"
#include <chrono>
#include <cinttypes>
#include <optional>
#include <vector>

namespace couchbase
{
/**
 * @since 1.0.0
 * @committed
 */
class search_facet_result
{
  public:
    virtual ~search_facet_result() = default;

    /**
     * @since 1.0.0
     * @internal
     */
    [[nodiscard]] virtual auto name() const -> const std::string& = 0;
    [[nodiscard]] virtual auto field() const -> const std::string& = 0;
    [[nodiscard]] virtual auto total() const -> std::uint64_t = 0;
    [[nodiscard]] virtual auto missing() const -> std::uint64_t = 0;
    [[nodiscard]] virtual auto other() const -> std::uint64_t = 0;
};
} // namespace couchbase
