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

#include "test_helper_integration.hxx"
#include <couchbase/operations/management/analytics.hxx>
#include <couchbase/operations/management/collection_create.hxx>
#include <couchbase/operations/management/scope_create.hxx>

TEST_CASE("integration: analytics query")
{
    test::utils::integration_test_guard integration;

    if (!integration.cluster_version().supports_analytics()) {
        return;
    }

    test::utils::open_bucket(integration.cluster, integration.ctx.bucket);

    auto dataset_name = test::utils::uniq_id("dataset");

    {
        couchbase::operations::management::analytics_dataset_create_request req{};
        req.dataset_name = dataset_name;
        req.bucket_name = integration.ctx.bucket;
        auto resp = test::utils::execute(integration.cluster, req);
        REQUIRE_FALSE(resp.ctx.ec);
    }

    {
        couchbase::operations::management::analytics_link_connect_request req{};
        auto resp = test::utils::execute(integration.cluster, req);
        REQUIRE_FALSE(resp.ctx.ec);
    }

    auto key = test::utils::uniq_id("key");
    auto test_value = test::utils::uniq_id("value");
    auto value = couchbase::utils::json::generate({ { "testkey", test_value } });
    {
        auto id = couchbase::document_id(integration.ctx.bucket, "_default", "_default", key);
        couchbase::operations::upsert_request req{ id, value };
        auto resp = test::utils::execute(integration.cluster, req);
        REQUIRE_FALSE(resp.ctx.ec);
    }

    SECTION("simple query")
    {
        couchbase::operations::analytics_response resp{};
        REQUIRE(test::utils::wait_until([&]() {
            couchbase::operations::analytics_request req{};
            req.statement = fmt::format(R"(SELECT testkey FROM `Default`.`{}` WHERE testkey = "{}")", dataset_name, test_value);
            resp = test::utils::execute(integration.cluster, req);
            return resp.rows.size() == 1;
        }));
        REQUIRE_FALSE(resp.ctx.ec);
        REQUIRE(resp.rows[0] == value);
        REQUIRE_FALSE(resp.meta.request_id.empty());
        REQUIRE_FALSE(resp.meta.client_context_id.empty());
        REQUIRE_FALSE(resp.meta.status.empty());
    }

    SECTION("positional params")
    {
        couchbase::operations::analytics_response resp{};
        REQUIRE(test::utils::wait_until([&]() {
            couchbase::operations::analytics_request req{};
            req.statement = fmt::format(R"(SELECT testkey FROM `Default`.`{}` WHERE testkey = ?)", dataset_name);
            req.positional_parameters.emplace_back(couchbase::json_string(couchbase::utils::json::generate(test_value)));
            resp = test::utils::execute(integration.cluster, req);
            return resp.rows.size() == 1;
        }));
        REQUIRE_FALSE(resp.ctx.ec);
        REQUIRE(resp.rows[0] == value);
    }

    SECTION("named params")
    {
        couchbase::operations::analytics_response resp{};
        REQUIRE(test::utils::wait_until([&]() {
            couchbase::operations::analytics_request req{};
            req.statement = fmt::format(R"(SELECT testkey FROM `Default`.`{}` WHERE testkey = $testkey)", dataset_name);
            req.named_parameters["testkey"] = couchbase::json_string(couchbase::utils::json::generate(test_value));
            resp = test::utils::execute(integration.cluster, req);
            return resp.rows.size() == 1;
        }));
        REQUIRE_FALSE(resp.ctx.ec);
        REQUIRE(resp.rows[0] == value);
    }

    SECTION("named params preformatted")
    {
        couchbase::operations::analytics_response resp{};
        REQUIRE(test::utils::wait_until([&]() {
            couchbase::operations::analytics_request req{};
            req.statement = fmt::format(R"(SELECT testkey FROM `Default`.`{}` WHERE testkey = $testkey)", dataset_name);
            req.named_parameters["$testkey"] = couchbase::json_string(couchbase::utils::json::generate(test_value));
            resp = test::utils::execute(integration.cluster, req);
            return resp.rows.size() == 1;
        }));
        REQUIRE_FALSE(resp.ctx.ec);
        REQUIRE(resp.rows[0] == value);
    }

    SECTION("raw")
    {
        couchbase::operations::analytics_response resp{};
        REQUIRE(test::utils::wait_until([&]() {
            couchbase::operations::analytics_request req{};
            req.statement = fmt::format(R"(SELECT testkey FROM `Default`.`{}` WHERE testkey = $testkey)", dataset_name);
            req.raw["$testkey"] = couchbase::json_string(couchbase::utils::json::generate(test_value));
            resp = test::utils::execute(integration.cluster, req);
            return resp.rows.size() == 1;
        }));
        REQUIRE_FALSE(resp.ctx.ec);
        REQUIRE(resp.rows[0] == value);
    }

    SECTION("consistency")
    {
        couchbase::operations::analytics_request req{};
        req.statement = fmt::format(R"(SELECT testkey FROM `Default`.`{}` WHERE testkey = "{}")", dataset_name, test_value);
        req.scan_consistency = couchbase::analytics_scan_consistency::request_plus;
        auto resp = test::utils::execute(integration.cluster, req);
        REQUIRE_FALSE(resp.ctx.ec);
        REQUIRE(resp.rows.size() == 1);
        REQUIRE(resp.rows[0] == value);
    }

    SECTION("readonly")
    {
        couchbase::operations::analytics_request req{};
        req.statement = fmt::format("DROP DATASET Default.`{}`", dataset_name);
        req.readonly = true;
        auto resp = test::utils::execute(integration.cluster, req);
        REQUIRE(resp.ctx.ec == couchbase::error::common_errc::internal_server_failure);
    }

    {
        couchbase::operations::management::analytics_dataset_drop_request req{};
        req.dataset_name = dataset_name;
        test::utils::execute(integration.cluster, req);
    }
}

TEST_CASE("integration: analytics scope query")
{
    test::utils::integration_test_guard integration;

    if (!integration.cluster_version().supports_analytics() || !integration.cluster_version().supports_collections()) {
        return;
    }

    test::utils::open_bucket(integration.cluster, integration.ctx.bucket);

    auto scope_name = test::utils::uniq_id("scope");
    auto collection_name = test::utils::uniq_id("collection");

    {
        couchbase::operations::management::scope_create_request req{ integration.ctx.bucket, scope_name };
        auto resp = test::utils::execute(integration.cluster, req);
        REQUIRE_FALSE(resp.ctx.ec);
        auto created = test::utils::wait_until_collection_manifest_propagated(integration.cluster, integration.ctx.bucket, resp.uid);
        REQUIRE(created);
    }

    {
        couchbase::operations::management::collection_create_request req{ integration.ctx.bucket, scope_name, collection_name };
        auto resp = test::utils::execute(integration.cluster, req);
        REQUIRE_FALSE(resp.ctx.ec);
        auto created = test::utils::wait_until_collection_manifest_propagated(integration.cluster, integration.ctx.bucket, resp.uid);
        REQUIRE(created);
    }

    {
        couchbase::operations::analytics_request req{};
        req.statement =
          fmt::format("ALTER COLLECTION `{}`.`{}`.`{}` ENABLE ANALYTICS", integration.ctx.bucket, scope_name, collection_name);
        auto resp = test::utils::execute(integration.cluster, req);
        REQUIRE_FALSE(resp.ctx.ec);
    }

    auto key = test::utils::uniq_id("key");
    auto test_value = test::utils::uniq_id("value");
    auto value = couchbase::utils::json::generate({ { "testkey", test_value } });
    {
        auto id = couchbase::document_id(integration.ctx.bucket, scope_name, collection_name, key);
        couchbase::operations::upsert_request req{ id, value };
        auto resp = test::utils::execute(integration.cluster, req);
        REQUIRE_FALSE(resp.ctx.ec);
    }

    couchbase::operations::analytics_response resp{};
    REQUIRE(test::utils::wait_until([&]() {
        couchbase::operations::analytics_request req{};
        req.statement = fmt::format(R"(SELECT testkey FROM `{}` WHERE testkey = "{}")", collection_name, test_value);
        req.bucket_name = integration.ctx.bucket;
        req.scope_name = scope_name;
        resp = test::utils::execute(integration.cluster, req);
        return resp.rows.size() == 1;
    }));
    REQUIRE_FALSE(resp.ctx.ec);
    REQUIRE(resp.rows[0] == value);

    {
        couchbase::operations::management::scope_drop_request req{ integration.ctx.bucket, scope_name };
        test::utils::execute(integration.cluster, req);
    }
}

couchbase::http_context
make_http_context()
{
    couchbase::topology::configuration config{};
    couchbase::query_cache query_cache{};
    couchbase::cluster_options cluster_options{};
    std::string hostname{};
    std::uint16_t port{};
    couchbase::http_context ctx{ config, cluster_options, query_cache, hostname, port };
    return ctx;
}

TEST_CASE("unit: analytics query")
{
    SECTION("priority true")
    {
        couchbase::io::http_request http_req;
        auto ctx = make_http_context();
        couchbase::operations::analytics_request req{};
        req.priority = true;
        auto ec = req.encode_to(http_req, ctx);
        REQUIRE_FALSE(ec);
        auto priority_header = http_req.headers.find("analytics-priority");
        REQUIRE(priority_header != http_req.headers.end());
        REQUIRE(priority_header->second == "-1");
    }

    SECTION("priority false")
    {
        couchbase::io::http_request http_req;
        auto ctx = make_http_context();
        couchbase::operations::analytics_request req{};
        req.priority = false;
        auto ec = req.encode_to(http_req, ctx);
        REQUIRE_FALSE(ec);
        REQUIRE(http_req.headers.find("analytics-priority") == http_req.headers.end());
    }
}