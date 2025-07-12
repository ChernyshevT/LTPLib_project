#pragma once

#include "typedefs.hxx"
#include "api_backend.hxx"

void check_errc (u32 errc);

typedef std::unordered_map<
	const char*,
	std::unique_ptr<void, std::function<void(void*)>>
> mholder_t;
