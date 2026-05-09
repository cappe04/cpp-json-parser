// Copyright (c) 2026 Casper Bené
// SPDX-License-Identifier: BSD-2-Clause

#include "json_parser/json.hpp"
#include "json_parser/detail/detail.hpp"

using namespace json;
using namespace json::detail;

Document::Document(std::unique_ptr<detail::JsonNode> root,
                   std::shared_ptr<const std::string> source)
: root(std::move(root)), source(source)
{}

Document::~Document() {
    if(root) {
        root->free();
    };
}

std::string Document::to_string() const noexcept {
    return root->to_string();
}

View Document::top_view() const noexcept {
    return View(root.get());
}