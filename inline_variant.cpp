#include <boost/variant.hpp>
#include <typeinfo>
#include <iostream>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/mpl/equal_to.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/contains.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/static_assert.hpp>
#include <boost/function_types/function_arity.hpp>
#include <boost/function_types/parameter_types.hpp>
#include <boost/function_types/result_type.hpp>
#include <boost/move/move.hpp>
#include <boost/fusion/include/map.hpp>
#include <boost/fusion/include/make_vector.hpp>
#include <boost/fusion/include/at_key.hpp>
#include <boost/fusion/algorithm/transformation/transform.hpp>
#include <utility>

#include "function_signature.hpp"

typedef boost::variant<int, char> IntChar;

struct function_arg_extractor
{
	template <typename FunctionType>
	struct apply
	{
		typedef typename boost::remove_const< typename boost::remove_reference<FunctionType>::type >::type bare_type;
		typedef typename signature_of<bare_type>::type normalized_function_type;
		typedef typename boost::function_types::function_arity<normalized_function_type>::type arity;
		typedef typename boost::function_types::parameter_types<normalized_function_type>::type parameter_types;
		typedef typename boost::function_types::result_type<normalized_function_type>::type result_type;

		BOOST_STATIC_ASSERT((arity::value == 1));

		typedef typename boost::mpl::front<parameter_types>::type parameter_type;
		typedef typename boost::remove_const< typename boost::remove_reference<parameter_type>::type >::type type;
	};
};

template <typename FunctionTypes>
struct get_function_args :
	boost::mpl::transform<FunctionTypes, function_arg_extractor>
{
};

struct pair_maker
{
    template<typename Sig>
    struct result;

    template <typename FunctionType>
    struct result<pair_maker(FunctionType)>
    {
		typedef typename function_arg_extractor::apply<FunctionType>::type arg_type;
		typedef boost::fusion::pair<arg_type, FunctionType> type;
    };

    template <typename FunctionType>
    typename result<pair_maker(FunctionType)>::type operator()(FunctionType a) const
    {
    	return boost::fusion::make_pair< typename result<pair_maker(FunctionType)>::arg_type >(a);
    }
};

template <typename ReturnType, typename... FunctionTypes >
struct generic_visitor : boost::static_visitor<ReturnType>
{
	typedef boost::mpl::vector<FunctionTypes...> function_types;
	typedef typename get_function_args<function_types>::type variant_types;
	typedef typename boost::mpl::transform<
		variant_types,
		function_types,
		boost::fusion::result_of::make_pair<boost::mpl::_1, boost::mpl::_2> >::type pair_list;
	typedef typename boost::fusion::result_of::as_map<pair_list>::type function_map;

	function_map fmap;

	generic_visitor(FunctionTypes... functions)
	:
		fmap(boost::fusion::as_map(boost::fusion::transform(boost::fusion::make_vector(functions...), pair_maker())))
	{
	}

	template <typename T>
	ReturnType operator()(T object) const {
		typedef typename boost::remove_const< typename boost::remove_reference<T>::type >::type bare_type;
		BOOST_STATIC_ASSERT((boost::mpl::contains<variant_types, bare_type>::type::value ));
		return boost::fusion::at_key<T>(fmap)(object);
	}
};

template <typename ReturnType, typename... FunctionTypes>
auto make_visitor(FunctionTypes... functions) -> generic_visitor< ReturnType, FunctionTypes... > {
	return generic_visitor< ReturnType, FunctionTypes... >(boost::forward<FunctionTypes>(functions)...);
}

struct Func1
{
	int operator()(int x) const {
        std::cout << "int " << x << std::endl;
        return 1;
	}
};

struct Func2
{
	int operator()(char x) const {
        std::cout << "char " << x << std::endl;
        return 2;
	}
};

int main() {
    IntChar v('a');
    //v = 3;
    int ret = boost::apply_visitor(make_visitor<int>(Func1(), Func2()), v);
    std::cout << ret << std::endl;
}
