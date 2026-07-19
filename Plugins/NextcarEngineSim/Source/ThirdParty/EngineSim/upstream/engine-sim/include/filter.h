#ifndef ATG_ENGINE_SIM_FILTER_H
#define ATG_ENGINE_SIM_FILTER_H

#if !defined(_MSC_VER) && !defined(__forceinline)
#define __forceinline inline __attribute__((always_inline))
#endif

class Filter {
    public:
        Filter();
        virtual ~Filter();

        virtual float f(float sample);
        virtual void destroy();
};

#endif /* ATG_ENGINE_SIM_FILTER_H */
