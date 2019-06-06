#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "singleton.h"
#include "function_traits.h"
#include "is_std_container.h"
#include "is_detected.h"

#include <QString>

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <utility>
#include <type_traits>

class ThreadPool
{
private:
    std::vector<std::thread> _threads;
    std::mutex _mutex;
    std::condition_variable _waitForNewTask;
    std::queue<std::function<void()>> _tasks;
    std::atomic<bool> _stop;
    std::atomic<int> _activeThreads;

public:
    explicit ThreadPool(const QString& threadNamePrefix = QStringLiteral("Worker"),
        unsigned int numThreads = std::thread::hardware_concurrency());
    virtual ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(const ThreadPool& other) = delete;
    ThreadPool& operator=(ThreadPool&& other) = delete;

    bool saturated() const { return _activeThreads >= static_cast<int>(_threads.size()); }
    bool idle() const { return _activeThreads == 0; }

    template<typename Fn, typename... Args> using ReturnType = typename std::invoke_result_t<Fn, Args...>;

    template<typename Fn, typename... Args> std::future<ReturnType<Fn, Args...>> makeFuture(Fn f, Args&&... args)
    {
        if(_stop)
            return std::future<ReturnType<Fn, Args...>>();

        auto taskPtr = std::make_shared<std::packaged_task<ReturnType<Fn, Args...>(Args...)>>(f);

        {
            std::unique_lock<std::mutex> lock(_mutex);
            _tasks.push(
                [taskPtr, args...]() mutable
                {
                    (*taskPtr)(std::forward<Args>(args)...);
                });
        }

        // Wake a thread up
        _waitForNewTask.notify_one();
        _activeThreads++;
        return taskPtr->get_future();
    }

private:
    // If the concurrent function returns a value, give the ResultsType class a std::vector _values
    // member, which contains the results from each thread
    template<typename ResultsVectorOrVoid, bool = std::is_void_v<ResultsVectorOrVoid>>
    class ResultMember;

    template<typename ResultsVectorOrVoid>
    class ResultMember<ResultsVectorOrVoid, true> {};

    template<typename ResultsVectorOrVoid>
    class ResultMember<ResultsVectorOrVoid, false>
    {
    protected:
        mutable std::vector<ResultsVectorOrVoid> _values;
    };

    template<typename ResultsVectorOrVoid> class ResultsType : public ResultMember<ResultsVectorOrVoid>
    {
        friend class ThreadPool;

    private:
        mutable std::vector<std::future<ResultsVectorOrVoid>> _futures;

        explicit ResultsType(std::vector<std::future<ResultsVectorOrVoid>>&& futures) :
            _futures(std::move(futures))
        {}

    public:
        void wait() const
        {
            for(auto& future : _futures)
            {
                future.wait();

                if constexpr(!std::is_void_v<ResultsVectorOrVoid>)
                    this->_values.emplace_back(std::move(future.get()));
            }
        }

        // This iterator allows the results to be iterated over in a single pass
        class iterator
        {
        private:
            template<typename T, bool = is_std_sequence_container_v<typename T::value_type>>
            struct impl {};

            // If the concurrent function's return type is a sequence container, collapse its
            // values down, so that from the iterator user's point of view, they work with a single
            // homogenous sequence, rather than a vector of sequences
            template<typename T>
            struct impl<T, true>
            {
                typename std::vector<T>::iterator _threadIt;
                typename std::vector<T>::iterator _threadEndIt;
                typename T::iterator _resultsIt;
                typename T::value_type::iterator _containerIt;

                using value_type = typename T::value_type::value_type;

                void increment()
                {
                    if(_containerIt != _resultsIt->end())
                        ++_containerIt;

                    while(_containerIt == _resultsIt->end())
                    {
                        if(_resultsIt != _threadIt->end())
                            ++_resultsIt;

                        if(_resultsIt != _threadIt->end())
                            _containerIt = _resultsIt->begin();
                        else do
                        {
                            if(_threadIt != _threadEndIt)
                                ++_threadIt;

                            if(_threadIt == _threadEndIt)
                                break;

                            _resultsIt = _threadIt->begin();
                            _containerIt = _resultsIt->begin();

                        } while(_resultsIt == _threadIt->end());

                        if(_threadIt == _threadEndIt)
                        {
                            _resultsIt = {};
                            _containerIt = {};
                            break;
                        }
                    }
                }

                impl(std::vector<T>& values, bool end)
                {
                    if(!end)
                    {
                        _threadIt = values.begin();
                        _threadEndIt = values.end();

                        if(_threadIt != _threadEndIt)
                        {
                            _resultsIt = _threadIt->begin();
                            while(_threadIt != _threadEndIt && _resultsIt == _threadIt->end())
                                increment();

                            if(_threadIt != _threadEndIt && _resultsIt != _threadIt->end())
                            {
                                _containerIt = _resultsIt->begin();
                                while(_threadIt != _threadEndIt && _containerIt == _resultsIt->end())
                                    increment();
                            }
                        }

                        if(_threadIt == _threadEndIt)
                        {
                            _resultsIt = {};
                            _containerIt = {};
                        }
                    }
                    else
                    {
                        _threadIt = _threadEndIt = values.end();
                        _resultsIt = {};
                        _containerIt = {};
                    }
                }

                auto& valueReference() const
                {
                    Q_ASSERT(_containerIt != _resultsIt->end());
                    return *_containerIt;
                }

                bool operator==(const impl& other) const
                {
                    if(_threadIt != other._threadIt)
                        return false;

                    if(_resultsIt != other._resultsIt)
                        return false;

                    return _containerIt == other._containerIt;
                }
            };

            template<typename T>
            struct impl<T, false>
            {
                typename std::vector<T>::iterator _threadIt;
                typename std::vector<T>::iterator _threadEndIt;
                typename T::iterator _resultsIt;

                using value_type = typename T::value_type;

                void increment()
                {
                    if(_resultsIt != _threadIt->end())
                        ++_resultsIt;

                    while(_resultsIt == _threadIt->end())
                    {
                        if(_threadIt != _threadEndIt)
                            ++_threadIt;

                        if(_threadIt == _threadEndIt)
                            break;

                        _resultsIt = _threadIt->begin();
                    }

                    if(_threadIt == _threadEndIt)
                        _resultsIt = {};
                }

                impl(std::vector<T>& values, bool end)
                {
                    if(!end)
                    {
                        _threadIt = values.begin();
                        _threadEndIt = values.end();

                        if(_threadIt != _threadEndIt)
                        {
                            _resultsIt = _threadIt->begin();
                            while(_threadIt != _threadEndIt && _resultsIt == _threadIt->end())
                                increment();
                        }

                        if(_threadIt == _threadEndIt)
                            _resultsIt = {};
                    }
                    else
                    {
                        _threadIt = _threadEndIt = values.end();
                        _resultsIt = {};
                    }
                }

                auto& valueReference() const
                {
                    Q_ASSERT(_resultsIt != _threadIt->end());
                    return *_resultsIt;
                }

                bool operator==(const impl& other) const
                {
                    if(_threadIt != other._threadIt)
                        return false;

                    return _resultsIt == other._resultsIt;
                }
            };

            impl<ResultsVectorOrVoid> _impl;

        public:
            using self_type = iterator;
            using value_type = typename impl<ResultsVectorOrVoid>::value_type;
            using reference = value_type&;
            using pointer = value_type*;
            using iterator_category = std::forward_iterator_tag;
            using difference_type = size_t;

            iterator(ResultsType* results, bool end) :
                _impl(results->_values, end)
            {}

            self_type operator++()
            {
                self_type i = *this;
                _impl.increment();
                return i;
            }

            auto& operator*() const
            {
                return _impl.valueReference();
            }

            bool operator!=(const self_type& other) const { return !operator==(other); }
            bool operator==(const self_type& other) const
            {
                return _impl == other._impl;
            }
        };

        template<typename T = ResultsVectorOrVoid>
        typename std::enable_if_t<!std::is_void_v<T>, iterator>
        begin() { return iterator(this, false); }

        template<typename T = ResultsVectorOrVoid>
        typename std::enable_if_t<!std::is_void_v<T>, iterator>
        end() { return iterator(this, true); }
    };

    template<typename Fn> using FirstArgumentType =
        typename function_traits<Fn>::template arg<0>::type;
    template<typename Fn> using LastArgumentType =
        typename function_traits<Fn>::template arg<function_traits<Fn>::arity - 1>::type;

    template<typename Fn> static constexpr bool HasThreadIndexArgument =
        function_traits<Fn>::arity == 2 &&
        std::is_same_v<LastArgumentType<Fn>, size_t>;

    template<typename It, typename Fn>
    class IteratorExecutor
    {
    private:
        size_t _index;

    public:
        IteratorExecutor() = default;

        void setIndex(size_t index) { _index = index; }

    protected:
        auto execute(Fn& f, It& it) const
        {
            // Fn argument is an iterator
            if constexpr(std::is_convertible_v<FirstArgumentType<Fn>, It>)
            {
                if constexpr(HasThreadIndexArgument<Fn>)
                    return f(it, _index);
                else
                    return f(it);
            }

            // Fn argument is an value/reference
            if constexpr(std::is_convertible_v<FirstArgumentType<Fn>, typename It::value_type>)
            {
                if constexpr(HasThreadIndexArgument<Fn>)
                    return f(*it, _index);
                else
                    return f(*it);
            }
        }
    };

    template<typename It, typename Fn, typename Result>
    class ResultsExecutor : public IteratorExecutor<It, Fn>
    {
    public:
        using ResultsVectorOrVoid = std::vector<Result>;

        ResultsVectorOrVoid operator()(It it, It last, Fn& f) const
        {
            ResultsVectorOrVoid values;
            values.reserve(std::distance(it, last));

            for(; it != last; ++it)
                values.emplace_back(std::move(this->execute(f, it)));

            return values;
        }
    };

    template<typename It, typename Fn>
    class ResultsExecutor<It, Fn, void> : public IteratorExecutor<It, Fn>
    {
    public:
        using ResultsVectorOrVoid = void;

        ResultsVectorOrVoid operator()(It it, It last, Fn& f) const
        {
            for(; it != last; ++it)
                this->execute(f, it);
        }
    };

    template<typename It, typename Fn> using Executor =
        ResultsExecutor<It, Fn, typename function_traits<Fn>::result_type>;

    template<typename It>
    using computeCostHint_t = decltype(std::declval<It>()->computeCostHint());

    template<typename It> static constexpr bool ItHasComputeCostHint =
        std::experimental::is_detected_v<computeCostHint_t, It>;

    template<typename It>
    class Coster
    {
    private:
        It _first;
        It _last;

    public:
        Coster(It first, It last) :
            _first(std::move(first)),
            _last(std::move(last))
        {}

        // When It->computeCostHint() does exist, we us it to hint how much computation
        // each element will cost, and balance the thread/work allocation accordingly
        // When there is no hinting available, each element is given equal weight
        int total()
        {
            if constexpr(ItHasComputeCostHint<It>)
            {
                int n = 0;

                for(auto it = this->_first; it != this->_last; ++it)
                    n += it->computeCostHint();

                return n;
            }
            else
                return std::distance(this->_first, this->_last);
        }

        int operator()(It it)
        {
            Q_UNUSED(it);

            if constexpr(ItHasComputeCostHint<It>)
                return it->computeCostHint();
            else
                return 1;
        }
    };

public:
    template<typename It, typename Fn> using Results =
        ResultsType<typename Executor<It, Fn>::ResultsVectorOrVoid>;

    enum ResultsPolicy
    {
        Blocking,
        NonBlocking
    };

    template<typename It, typename Fn>
    auto concurrent_for(It first, It last, Fn f, ResultsPolicy resultsPolicy = Blocking)
    {
        Coster<It> coster(first, last);

        const auto totalCost = coster.total(); Q_ASSERT(totalCost > 0);
        const auto numThreads = static_cast<int>(_threads.size());
        const auto costPerThread = totalCost / numThreads +
                ((totalCost % numThreads) ? 1 : 0);

        static_assert(std::is_convertible_v<FirstArgumentType<Fn>, It> ||
            std::is_convertible_v<FirstArgumentType<Fn>, typename It::value_type>,
            "Fn's argument must be an It or an It::value_type");

        static_assert(function_traits<Fn>::arity == 1 || HasThreadIndexArgument<Fn>,
            "Fn's (optional) second index argument must be size_t");

        Executor<It, Fn> executor;
        std::vector<std::future<typename Executor<It, Fn>::ResultsVectorOrVoid>> futures;
        size_t threadIndex = 0;

        for(It it = first; it != last;)
        {
            It threadLast = it;
            int cost = 0;
            do
            {
                cost += coster(threadLast);
                ++threadLast;
            }
            while(threadLast != last && cost < costPerThread);

            Q_ASSERT(threadIndex < _threads.size());

            futures.emplace_back(makeFuture([executor, it, threadLast, f, threadIndex]() mutable
            {
                executor.setIndex(threadIndex);
                return executor(it, threadLast, f);
            }));

            it = threadLast;
            threadIndex++;
        }

        auto results = Results<It, Fn>(std::move(futures));

        if(resultsPolicy == Blocking)
            results.wait();

        return results;
    }
};

class ThreadPoolSingleton : public ThreadPool, public Singleton<ThreadPoolSingleton> {};

template<typename Fn, typename... Args>
auto execute_on_threadpool(Fn&& f, Args&&... args)
{
    return S(ThreadPoolSingleton)->makeFuture(std::forward<Fn>(f), args...);
}

template<typename It, typename Fn>
auto concurrent_for(It first, It last, Fn&& f, ThreadPool::ResultsPolicy resultsPolicy = ThreadPool::Blocking)
{
    return S(ThreadPoolSingleton)->concurrent_for(first, last, std::forward<Fn>(f), resultsPolicy);
}

#endif // THREADPOOL_H
