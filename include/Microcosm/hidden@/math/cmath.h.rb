funcs = [
['abs',         [['auto','x']]],
['arg',         [['auto','x']]],
['fabs',        [['auto','x']]],
['fma',         [['auto','x'], ['auto','y'], ['auto','z']]],
['fmin',        [['auto','x'], ['auto','y']]],
['fmax',        [['auto','x'], ['auto','y']]],
['fdim',        [['auto','x'], ['auto','y']]],
['fmod',        [['auto','x'], ['auto','y']]],
['remquo',      [['auto','x'], ['auto','y'], ['int*','q']]],
['remainder',   [['auto','x'], ['auto','y']]],
['nearbyint',   [['auto','x']]],
['floor',       [['auto','x']]],
['ceil',        [['auto','x']]],
['trunc',       [['auto','x']]],
['round',       [['auto','x']]],
['rint',        [['auto','x']]],
['lrint',       [['auto','x']]],
['llrint',      [['auto','x']]],
['lround',      [['auto','x']]],
['llround',     [['auto','x']]],
['frexp',       [['auto','x'], ['int*','p']]],
['ldexp',       [['auto','x'], ['int','p']]],
['logb',        [['auto','x']]],
['ilogb',       [['auto','x']]],
['scalbn',      [['auto','x'], ['int','p']]],
['scalbln',     [['auto','x'], ['long','p']]],
['modf',        [['auto','x'], ['auto*','p']]],
['nextafter',   [['auto','x'], ['auto','y']]],
['nexttoward',  [['auto','x'], ['long double','y']]],
['copysign',    [['auto','x'], ['auto','y']]],
['signbit',     [['auto','x']]],
['isnan',       [['auto','x']]],
['isinf',       [['auto','x']]],
['isfinite',    [['auto','x']]],
['isnormal',    [['auto','x']]],
['exp',         [['auto','x']]],
['log',         [['auto','x']]],
['exp2',        [['auto','x']]],
['log2',        [['auto','x']]],
['log10',       [['auto','x']]],
['expm1',       [['auto','x']]],
['log1p',       [['auto','x']]],
['pow',         [['auto','x'], ['auto','y']]],
['sqrt',        [['auto','x']]],
['cbrt',        [['auto','x']]],
['hypot',       [['auto','x'], ['auto','y']]],
['erf',         [['auto','x']]],
['erfc',        [['auto','x']]],
['lgamma',      [['auto','x']]],
['tgamma',      [['auto','x']]],
['sin',         [['auto','x']]],
['cos',         [['auto','x']]],
['tan',         [['auto','x']]],
['asin',        [['auto','x']]],
['acos',        [['auto','x']]],
['atan',        [['auto','x']]],
['atan2',       [['auto','y'], ['auto','x']]],
['sinh',        [['auto','x']]],
['cosh',        [['auto','x']]],
['tanh',        [['auto','x']]],
['asinh',       [['auto','x']]],
['acosh',       [['auto','x']]],
['atanh',       [['auto','x']]]
]

puts <<STR
namespace mi {

STR

for func in funcs
    funcname = func[0]
    args1 = []
    args2 = []
    for arg in func[1]
        args1 << "#{arg[0]} #{arg[1]}"
        args2 << "#{arg[1]}"
    end
    args1 = args1.join ", "
    args2 = args2.join ", "
    puts <<STR
[[gnu::always_inline]] 
inline auto #{funcname}(#{args1}) -> decltype(std::#{funcname}(#{args2})) {
    return std::#{funcname}(#{args2});
}

STR
end

puts <<STR
} // namespace mi

STR
