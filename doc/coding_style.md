# Coding Style Guidelines


### Naming
#### Style
* Guideline:
    ```
    1) class UpperCamelCase;

    2) void lowerCamelCase(int snake_case);

    3) void UpperCamelCase::lowerCamelCase();

    4) int snake_case;

    5) int UpperCamelCase::m_snake_case;

    6) template <typename UpperCamelCase>
       concept snake_case = ...;

    7) using snake_case_type = ...;

    8) namespace snake_case;

    9) #define ALLUPPERCASE;
    ```
* Example:
    ```cpp
    #define MACRO

    namespace my_namespace {
        constexpr int static_variable = 0;

        void incrementVariable(int& param_var);

        template <typename... Poses>
        concept valid_positions = (std::same_as<std::size_t, Poses> || ...);

        template <typename Type, int Size>
        class MyAwesomeArray {
        public:
            using array_value_type = Type;
            
            const int m_array_size = Size;

            void writeSlot(const int& param_var);
        };
    }
    ```
#### Explanation
1. Classes, structs, and unions should be named with upper-camel-case;
2. Functions should be named with lower-camel-case; Parameter names should be named with snake-case;
3. Rules for functions also apply for class methods;
4. Variables should be named with snake-case;
5. Class member variables should be named with snake-case, with prefix `m_`;
6. Type-name for templates should be named with upper-camel-case; Concept names should be named with snake-case;
7. Type alias should be named with snake-case, with suffix `_type`;
8. Namespace should be named with snake-case;
9. Macro names should be named with all-upper-case;


### Functions
#### Style
* Guideline:
    ```
    <explicit template> <attributes> <specifiers>
    auto function-name(<parameter list>) <specifiers> <qualifiers>
        -> return-type
    {
        function-body;
    }

    NOTE: Item wrapped around by <...> is optional.
    ```
* Example:
    ```cpp
    template <typename T> [[nodiscard]] static inline constexpr
    auto promote(const T n) noexcept
        -> make_promoted_t<T>
    {
        return n;
    }
    ```
    ```cpp
    [[nodiscard]] static inline constexpr
    auto pos(const std::unsigned_integral auto position) noexcept
        -> std::size_t
    {
        return static_cast<std::size_t>(position);
    }
    ```
    ```cpp
    template <a_very_very_long_concept_name    Array    ,
              another_long_concept_name<Array> HashTable,
              a_short_concept_name             Memory    > [[nodiscard]] inline constexpr
    auto aVeryVeryLongFunctionName(const std::string& my_str    ,
                                   HashTable&&        hash_table,
                                   Memory&&           memory     ) const noexcept &&
        -> my_namespace::my_custom_type
    {
        /* function-body */
    }
    ```
#### Explanation
* First line consists of explicit template declaration, attributes, and specifiers;
* Second line consists of `auto` which symbolizes start of function declaration, function name, parameter list, more specifiers and qualifiers;
* Third line consists of `->` which introduces the return type;
* Forth line should and only should consists of one `{` which introduces the function body;
* Last line should and only should consists of the ending `}`;
* In case of very long template declaration or parameter list, each parameter should be split into their own line;
* If parameters are split, type names should be aligned, parameter names should be aligned, and commas should be aligned;
* The closing bracket (`>` or `)`) should be aligned with commas, but one space to the right;
* Attributes, qualifiers, and specifiers should follow immediately after the closing brackets;


### Class Section Separation
#### Style
* Guideline:
    ```cpp
    class C
    {
    public:
        // Type aliasing (using/typedef declarations)
        // public constants (const specified variable)
        // Maybe: static member variables

    public:
        // special class member functions
        // e.g. ctor, dtor, move/copy ctor, move/copy optor

    public:
        // public methods

    private:
        // private helper functions

    public:
        // non-static public member variables

    protected:
        // non-static protected member variables

    private:
        // non-static private member variables
    };
    ```
#### Explanation
* Class is consists of many sections.
* Member access specifiers (aka. Labels) should be used to separate different sections.
* DO NOT use comments to separate.


### Constructor
#### Style
* Guideline:
    ```
    <explicit template> <attributes> <specifiers>
    name(<parameter list>)
        : <var1>{ <initial value> }
        , <var2>{ <initial value> }
        , <var2>{ <initial value> }
    {
        // code
    }
    ```
* Example:
    ```cpp
    template <std::unsigned_integral UINT> [[nodiscard]] constexpr explicit
    Shape(const UINT width, const UINT height) noexcept(false)
        : m_width { width  }
        , m_height{ height }
    {
        // Perform some error checking on inputs
        if (m_width < 0 || m_height < 0)
        {
            throw std::runtime_error("Unexpected error!");
        }
    }
    ```
#### Explanation
* Please strictly follow above guidelines!