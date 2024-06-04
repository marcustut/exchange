use matcher::{Symbol, TradeEvent};
use sbe::{
    message_header_codec::{self, MessageHeaderDecoder},
    trade_codec::{self, TradeDecoder, TradeEncoder},
    ReadBuf, SbeErr, WriteBuf,
};

#[derive(Debug, thiserror::Error)]
pub enum Error {
    #[error("Symbol {0} not found")]
    SymbolNotFound(Symbol),

    #[error("SBE: {0}")]
    Sbe(#[from] SbeErr),

    #[error("SBE: Invalid template ID {0}")]
    SbeInvalidTemplateId(u16),
}

type Result<T> = core::result::Result<T, Error>;

pub mod decode;
pub use decode::*;

pub mod encode;
pub use encode::*;

pub mod types;

#[cfg(test)]
mod tests {
    use super::*;
    use decode::TryDecode;
    use encode::{Encode, TryEncodeWithCtx};

    #[test]
    fn test_f64_decode_encode() {
        let encoder = encode::Encoder::new();

        let tcs = vec![0.0, 1.0, -1.0, 1.5, -1.5, 12.5, -12.5, 0.123124];

        for tc in tcs {
            let (mantissa, exponent, sign) = encoder.encode(tc);
            print!("exponent: {exponent}\nmantissa: {mantissa}\nsign: {sign}\n");
            let b = sign as f64 * mantissa as f64 * 2f64.powi(exponent.into());
            assert_eq!(tc, b);
        }
    }

    #[test]
    fn test_trade_encode() {
        let encoder = encode::Encoder::new();
        let decoder = decode::Decoder {};

        let tcs = vec![TradeEvent {
            size: 1230,
            price: 17239821,
            side: matcher::Side::Bid,
            buyer_order_id: 123,
            seller_order_id: 124,
        }];

        for tc in tcs {
            let encoded = encoder
                .try_encode(&tc, (1, Symbol::BTCUSDT, chrono::Utc::now()))
                .unwrap();
            let trade: types::Trade = decoder.try_decode(encoded.as_slice()).unwrap();

            assert_eq!(tc.side, trade.taker_side);
            assert_eq!(tc.price, trade.price);
            assert_eq!(tc.size, trade.size);
        }
    }
}
