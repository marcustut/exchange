use intmap::IntMap;

#[derive(Debug, Clone, Copy)]
#[repr(u64)]
pub enum Symbol {
    BTCUSDT = 0,
}

impl From<u64> for Symbol {
    fn from(value: u64) -> Self {
        match value {
            0 => Self::BTCUSDT,
            _ => unreachable!(),
        }
    }
}

pub struct BookMap<V> {
    map: IntMap<V>,
}

impl<V> BookMap<V> {
    pub fn new() -> Self {
        Self { map: IntMap::new() }
    }

    pub fn with_capacity(n: usize) -> Self {
        Self {
            map: IntMap::with_capacity(n),
        }
    }

    pub fn insert(&mut self, symbol: Symbol, value: V) -> Option<V> {
        self.map.insert(symbol as u64, value)
    }

    pub fn get(&self, symbol: Symbol) -> Option<&V> {
        self.map.get(symbol as u64)
    }

    pub fn get_mut(&mut self, symbol: Symbol) -> Option<&mut V> {
        self.map.get_mut(symbol as u64)
    }

    pub fn remove(&mut self, symbol: Symbol) -> Option<V> {
        self.map.remove(symbol as u64)
    }
}
