#ifndef _IO_MEMORY_HEADER
#define _IO_MEMORY_HEADER
#include <memory>

////////////////////////////////////////////////////////////////////////////////

class mem_holder {

	struct {
		std::vector<std::tuple<void**, size_t>>
			reqs;
		std::unique_ptr<std::byte[], std::function<void(void*)>>
			data;
		size_t
			size{0};
	} m;

public:
	#define MEM_ALIGN 32
	template<typename tp>
	mem_holder& req(tp **pptr, size_t n_elements) {
		void **p = reinterpret_cast<void**>(pptr);
		size_t k = MEM_ALIGN*((n_elements*sizeof(tp))/MEM_ALIGN + 1);

		if (not m.data) {
			m.reqs.emplace_back(p, k);
		} else {
			throw std::runtime_error("mem_holder: req error");
		} m.size += k;
		
		return *this;
	}
	
	inline
	size_t get_size () const {
		return m.size;
	}
	
	inline
	mem_holder& alloc(void) {
		size_t nsize{0};
		for(auto& [p, n] : m.reqs) {
			nsize += n;
		}
		
		m.data = {(std::byte*)malloc(nsize), &free};
		if (not m.data) {
			throw std::runtime_error("mem_holder: alloc error");
		}
		memset(&m.data[0], 0, nsize);

		size_t shift{0};
		for(auto& [p, n] : m.reqs) {
			if (p) {
				*p  = &(m.data[shift]);
			}
			shift += n;
		}

		return *this;
	}
	
};

#endif
