// use strum::EnumCount;
// use strum_macros::{Display, EnumCount, EnumIter, EnumString, FromRepr};

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
pub struct Symbol(pub u64);

impl Symbol {
    pub const BTCUSDT: Self = Self(0);
    pub const ETHUSDT: Self = Self(1);
    pub const ADAUSDT: Self = Self(2);
}

impl std::fmt::Display for Symbol {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(
            f,
            "{}",
            match *self {
                Symbol::BTCUSDT => "BTCUSDT".to_string(),
                Symbol::ETHUSDT => "ETHUSDT".to_string(),
                Symbol::ADAUSDT => "ADAUSDT".to_string(),
                _ => format!("UNKNOWN({})", self.0),
            },
        )
    }
}

impl From<Symbol> for u64 {
    fn from(value: Symbol) -> Self {
        value.0
    }
}

// #[derive(Debug, Clone, Copy, EnumIter, EnumCount, FromRepr, EnumString, Display)]
// #[repr(u64)]
// pub enum Symbol {
//     BTCUSDT = 0,
//     ETHUSDT = 1,
//     ADAUSDT = 2,
// }

#[cfg(feature = "random")]
impl rand::distributions::Distribution<Symbol> for rand::distributions::Standard {
    fn sample<R: rand::Rng + ?Sized>(&self, rng: &mut R) -> Symbol {
        Symbol(rng.gen_range(0..3).try_into().unwrap())
    }
}

#[derive(Debug)]
pub struct SymbolTable<V> {
    // pub(crate) table: [Option<V>; Symbol::COUNT],
    pub(crate) table: intmap::IntMap<V>,
}

impl<V> SymbolTable<V> {
    pub fn new() -> Self {
        Self {
            // table: core::array::from_fn(|_| None),
            table: intmap::IntMap::with_capacity(20),
        }
    }

    pub fn insert(&mut self, symbol: Symbol, value: V) -> Option<V> {
        // let pv = self.table[symbol as usize].take();
        // self.table[symbol as usize] = Some(value);
        // pv
        self.table.insert(symbol.into(), value)
    }

    pub fn get(&self, symbol: Symbol) -> Option<&V> {
        // self.table[symbol as usize].as_ref()
        self.table.get(symbol.into())
    }

    pub fn get_mut(&mut self, symbol: Symbol) -> Option<&mut V> {
        // self.table[symbol as usize].as_mut()
        self.table.get_mut(symbol.into())
    }

    pub fn remove(&mut self, symbol: Symbol) -> Option<V> {
        // self.table[symbol as usize].take()
        self.table.remove(symbol.into())
    }
}
