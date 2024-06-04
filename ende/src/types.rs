#[derive(Debug, Clone)]
pub struct Trade {
    pub trade_id: u64,
    pub symbol_id: u64,
    pub price: u64,
    pub size: u64,
    pub taker_side: matcher::Side,
    pub buy_order_id: u64,
    pub sell_order_id: u64,
    pub timestamp: i64,
}

#[derive(Debug, Clone)]
pub enum Message {
    Trade(Trade),
}
