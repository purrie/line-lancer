#ifndef OPTIONAL_H_
#define OPTIONAL_H_

#define makeOptional(type, name) typedef struct {\
        type value;\
        bool has_value;\
    } Optional ## name

#define makeOptionals(type, size, name) typedef struct {\
        type value[size];\
        bool has_value;\
    } Optionals ## name


#endif // OPTIONAL_H_
