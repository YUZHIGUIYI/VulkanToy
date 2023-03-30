//
// Created by ZHIKANG on 2023/3/30.
//

#pragma

#include <VulkanToy/Core/Base.h>

namespace VT::Events
{
    /**
     * Handles the event flow between subscribers and publishers
     * @param Args Variable amount of parameters which can be exchanged with the delegate
     */
    template <class ... Args>
    class EventDelegate
    {
    public:
        EventDelegate() = default;

        /**
         * Subscribes a method/function to the event delegate which wants to receive the arguments specified in Args.
         * @param handle A method/function which has the required Args as parameters
         * @return A handle which can later be used to unsubscribe from the event delegate.
         * @note To subscribe a method of an object to the event delegate use std::bind().
		 * If the handle is invalidated, the object will be unsubscribed automatically.
         */
        uint32_t subscribe(std::function<void(Args ...)> func);

        /**
         * Unsubscribes a method/function from the event delegate.
         * @param subscriberID The handle which was returned previously at the time of subscription.
         */
        void unsubscribe(uint32_t subscriberID);

        /**
         * Publishes Args arguments to the subscribers.
         * @param args The arguments which should be published
         */
        void broadcast(Args ... args);

    private:
        std::unordered_map<uint32_t, std::function<void(Args ...)>> handler;
        uint32_t count = 0;
        std::mutex mutex;
    };

    template <class ... Args>
    uint32_t EventDelegate<Args ...>::subscribe(std::function<void(Args...)> func)
    {
        std::unique_lock<std::mutex> guard{ mutex };
        auto id = count++;
        handler[id] = func;
        return id;
    }

    template <class ... Args>
    void EventDelegate<Args ...>::unsubscribe(uint32_t subscriberID)
    {
        // TODO: fix bug???
        //if (auto it = handler.find(subscriberID); it != handler.end())
        std::unique_lock<std::mutex> guard{ mutex };
        handler.erase(subscriberID);
    }

    template<class ... Args>
    void EventDelegate<Args ...>::broadcast(Args ...args)
    {
        std::unique_lock<std::mutex> guard{ mutex };

        auto it = handler.begin();

        auto copy = std::vector<std::function<void(Args ...)>>{};
        std::transform(handler.begin(), handler.end(), std::back_inserter(copy),
                        [](const auto& it) -> std::function<void(Args ...)> { return it.second; });
        guard.unlock();

        for (auto& handle : copy)
        {
            handle(args ...);
        }
    }
}



































