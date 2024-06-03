use strum::EnumCount;
use strum_macros::{Display, EnumCount, EnumIter, EnumString, FromRepr};

#[derive(Debug, Clone, Copy, EnumIter, EnumCount, FromRepr, EnumString, Display)]
#[repr(u64)]
pub enum Symbol {
    BTCUSDT = 0,
    ETHUSDT = 1,
    ADAUSDT = 2,
}

#[cfg(feature = "random")]
impl rand::distributions::Distribution<Symbol> for rand::distributions::Standard {
    fn sample<R: rand::Rng + ?Sized>(&self, rng: &mut R) -> Symbol {
        Symbol::from_repr(rng.gen_range(0..Symbol::COUNT).try_into().unwrap()).unwrap()
    }
}

#[derive(Debug)]
pub struct SymbolTable<V> {
    table: [Option<V>; Symbol::COUNT],
}

impl<V> SymbolTable<V> {
    pub fn new() -> Self {
        Self {
            table: core::array::from_fn(|_| None),
        }
    }

    pub fn insert(&mut self, symbol: Symbol, value: V) -> Option<V> {
        let pv = self.table[symbol as usize].take();
        self.table[symbol as usize] = Some(value);
        pv
    }

    pub fn get(&self, symbol: Symbol) -> Option<&V> {
        self.table[symbol as usize].as_ref()
    }

    pub fn get_mut(&mut self, symbol: Symbol) -> Option<&mut V> {
        self.table[symbol as usize].as_mut()
    }

    pub fn remove(&mut self, symbol: Symbol) -> Option<V> {
        self.table[symbol as usize].take()
    }
}
