use matcher::{Order, Symbol, SymbolMetadata};

pub mod axum;

pub enum Request {
    Order((Symbol, Order)),
    AddSymbol((Symbol, SymbolMetadata)),
    None,
}

impl Default for Request {
    fn default() -> Self {
        Self::None
    }
}
