#include <boost/coroutine2/all.hpp>

namespace co
{
    struct ContextScopedGuard {};

    template< typename traitsT >
    class my_fixedsize_stack {
        private:
            uint32_t & protect_page_;
            std::size_t size_;

        public:
            typedef traitsT traits_type;

            explicit my_fixedsize_stack(uint32_t & protect_page, std::size_t size)
                : protect_page_(protect_page), size_(size)
            {
                BOOST_ASSERT(traits_type::minimum_size() <= size_);
                BOOST_ASSERT(traits_type::is_unbounded() || (traits_type::maximum_size()>= size_));
            }

            boost::context::stack_context allocate() {
                void * vp = StackAllocator::get_malloc_fn()(size_);
                if (!vp) throw std::bad_alloc();

#if __linux__
                uint32_t protect_page = StackAllocator::get_protect_stack_page();
                if (protect_page)
                    if (StackAllocator::protect_stack(vp, size_, protect_page))
                        protect_page_ = protect_page;
#endif

                boost::context::stack_context sctx;
                sctx.size = size_;
                sctx.sp = static_cast<char *>(vp) + sctx.size;
#if defined(BOOST_USE_VALGRIND)
                sctx.valgrind_stack_id = VALGRIND_STACK_REGISTER(sctx.sp, vp);
#endif
                return sctx;
            }

            void deallocate(boost::context::stack_context & sctx) BOOST_NOEXCEPT_OR_NOTHROW {
                BOOST_ASSERT(sctx.sp);
                BOOST_ASSERT(traits_type::minimum_size() <= sctx.size);
                BOOST_ASSERT(traits_type::is_unbounded() || (traits_type::maximum_size()>= sctx.size));

#if defined(BOOST_USE_VALGRIND)
                VALGRIND_STACK_DEREGISTER(sctx.valgrind_stack_id);
#endif

                void * vp = static_cast<char *>(sctx.sp) - sctx.size;
#if __linux__
                if (protect_page_)
                    StackAllocator::unprotect_stack(vp, protect_page_);
#endif
                StackAllocator::get_free_fn()(vp);
            }
    };

    typedef my_fixedsize_stack<boost::context::stack_traits>  my_fixedsize_stack_allocator;

    class Context
    {
        public:
            Context(std::size_t stack_size, std::function<void()> const& fn)
                : protect_page_(0), ctx_(my_fixedsize_stack_allocator(protect_page_, stack_size),
                        [=](::boost::coroutines2::coroutine<void>::push_type& yield)
                        {
                            yield();
                            this->yield_ = &yield;
                            fn();
                        }), yield_(nullptr)
            {
            }

            ALWAYS_INLINE bool SwapIn()
            {
                ctx_();
                return true;
            }

            ALWAYS_INLINE bool SwapOut()
            {
                (*yield_)();
                return true;
            }

        private:
            uint32_t protect_page_;
            ::boost::coroutines2::coroutine<void>::pull_type ctx_;
            ::boost::coroutines2::coroutine<void>::push_type *yield_;
    };

} //namespace co
