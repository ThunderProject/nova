use proc_macro::TokenStream;
use quote::{format_ident, quote};
use syn::{FnArg, ItemFn, PatIdent, ReturnType, parse_macro_input, token::Type};

#[proc_macro_attribute]
pub fn authenticated_command(_attr: TokenStream, item: TokenStream) -> TokenStream {
    let input_fn = parse_macro_input!(item as ItemFn);
    let function_name = &input_fn.sig.ident;
    let visibility = &input_fn.vis;
    let asyncness = &input_fn.sig.asyncness;
    let function_block = &input_fn.block;

    let mut original_inputs = Vec::new();
    for arg in &input_fn.sig.inputs {
        match arg {
            FnArg::Typed(pat) => original_inputs.push(pat.clone()),
            FnArg::Receiver(_) => {
                panic!("#[authenticated_command] does not support &self functions.");
            }
        }
    }

    let mut expanded_inputs = original_inputs.clone();
    expanded_inputs.push(syn::parse_quote!(
        state: tauri::State<'_, crate::auth_state::auth_state::AuthState>
    ));

    let arg_names: Vec<_> = original_inputs
        .iter()
        .map(|p| {
            let syn::Pat::Ident(PatIdent { ident, .. }) = &*p.pat else {
                panic!("Unsupported pattern in function parameters.");
            };
            quote!(#ident)
        })
        .collect();

    let ret_type = match &input_fn.sig.output {
        ReturnType::Type(_, ret_type) => quote!(#ret_type),
        ReturnType::Default => {
            panic!("#[authenticated_command] requires the function to return Result<T, E>");
        }
    };

    let internal_fn_name = format_ident!("__{}_internal", function_name);

    let output = quote! {
        #visibility #asyncness fn #internal_fn_name(#( #original_inputs ),*) -> #ret_type {
            #function_block
        }

        #[tauri::command]
        #visibility #asyncness fn #function_name(#( #expanded_inputs ),*) -> #ret_type {
            if !state.authenticated.load(std::sync::atomic::Ordering::Relaxed) {
                return Err("Access denied.".to_string());
            }

            #internal_fn_name(#( #arg_names ),*).await
        }
    };

    output.into()
}
