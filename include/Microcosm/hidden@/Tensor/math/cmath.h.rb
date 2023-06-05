funcs = [
'abs',        
'arg',        
'real',       
'imag',       
'conj',       
'norm',       
'dual',       
'sign',
'softSign',
'softPlus',
'saturate',
'fabs',       
'nearbyint',  
'floor',      
'ceil',       
'trunc',      
'round',      
'rint',       
'lrint',      
'llrint',     
'lround',     
'llround',    
'signbit',    
'isnan',      
'isinf',      
'isfinite',   
'isnormal',   
'exp',        
'log',        
'exp2',       
'log2',       
'log10',      
'expm1',      
'log1p',      
'sqrt',       
'cbrt',       
'erf',        
'erfc',       
'lgamma',     
'tgamma',     
'sin',        
'cos',        
'tan',        
'asin',       
'acos',       
'atan',       
'sinh',       
'cosh',       
'tanh',       
'asinh',      
'acosh',      
'atanh',      
'nextFloat', 
'prevFloat', 
'fastFloor', 
'fastCeil',  
'fastTrunc', 
'fastRound', 
'fastFract',      
'sinPi',      
'cosPi',      
'erfInverse'
]

puts <<STR
\#pragma once

namespace mi {

STR

for func in funcs
  puts <<STR
template <typename Expr>
requires(concepts::tensor<Expr>) 
[[nodiscard, strong_inline]] constexpr auto #{func}(Expr&& expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) {  //
      return #{func}(expr(i)); 
    });
}

STR
end

for func in ['min', 'max', 'copysign']
    puts <<STR
template <typename ExprA, typename ExprB> requires(concepts::tensor_op2<ExprA, ExprB>) 
[[nodiscard, strong_inline]] constexpr auto #{func}(ExprA&& exprA, ExprB&& exprB) {
  auto shape = [&]() constexpr {
    if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>) {
      return equalShapes(exprA.shape, exprB.shape);
    } else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>) {
      return exprA.shape;
    } else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>) {
      return exprB.shape;
    }
  };
  return TensorLambda( //
    shape(),
    [exprA = capture_in_tensor_lambda(std::forward<ExprA>(exprA)),
     exprB = capture_in_tensor_lambda(std::forward<ExprB>(exprB))](auto i) constexpr {
      if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>) {
        return #{func}(exprA(i), exprB(i));
      } else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>) {
        return #{func}(exprA(i), exprB);
      } else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>) {
        return #{func}(exprA, exprB(i));
      }
    });
}

STR
end

puts <<STR

} // namespace mi
STR
