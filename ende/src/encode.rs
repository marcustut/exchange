use sbe::{
    ping_codec::{self, PingEncoder},
    server_time_codec::{self, ServerTimeEncoder},
};

use super::*;

pub trait Encode<In, Out> {
    fn encode(&self, data: In) -> Out;
}

pub trait TryEncode<In, Out> {
    fn try_encode(&self, data: In) -> Result<Out>;
}

pub trait TryEncodeWithCtx<In, Out> {
    type Ctx;

    fn try_encode(&self, data: In, ctx: Self::Ctx) -> Result<Out>;
}

#[derive(Debug)]
pub struct Encoder {}

pub struct Ping {}
pub struct ServerTime {}

impl Encoder {
    pub fn new() -> Self {
        Self {}
    }
}

impl Encode<f64, (u64, i16, i8)> for Encoder {
    // This method encodes a `f64` into (mantissa, exponent) pair which does not
    // require floating point operations to decode. It can be recovered by
    // `sign * mantissa * 2^exponent`.
    //
    // See: <https://stackoverflow.com/questions/75430066/why-does-integer-decode-in-rust-work-that-way>
    fn encode(&self, data: f64) -> (u64, i16, i8) {
        // IEEE754 64-bit floating point representation in bits:
        //
        // 1  bit for sign     (Bit 63)
        // 11 bit for exponent (Bit 62 - 52)
        // 52 bit for mantissa (Bit 51 - 0)
        //
        // Hence, the bits are arranged as [63, {62-52}, {51-0}]

        const MANTISSA_BIT_MASK: u64 = 0xfffffffffffff; // 52 one bits
        const MANTISSA_BIT_SIZE: i16 = 52;
        const EXPONENT_BIT_MASK: u64 = 0x7ff; // 11 one bits
        const EXPONENT_BIAS: i16 = 0x3ff; // 10 one bits

        let bits: u64 = data.to_bits();
        let mut exponent = ((bits >> MANTISSA_BIT_SIZE) & EXPONENT_BIT_MASK) as i16;
        let mantissa = if exponent == 0 {
            (bits & MANTISSA_BIT_MASK) << 1
        } else {
            (bits & MANTISSA_BIT_MASK) | 0x10000000000000 // add 1 bit at bit 53
        };

        exponent -= EXPONENT_BIAS + MANTISSA_BIT_SIZE; // exponent bias + mantissa shift

        (mantissa, exponent, if bits >> 63 == 0 { 1 } else { -1 })
    }
}

impl Encode<Ping, Vec<u8>> for Encoder {
    fn encode(&self, _data: Ping) -> Vec<u8> {
        let mut buffer = vec![0u8; ping_codec::SBE_BLOCK_LENGTH as usize];
        let mut ping = PingEncoder::default();
        ping = ping.wrap(
            WriteBuf::new(buffer.as_mut_slice()),
            message_header_codec::ENCODED_LENGTH,
        );
        ping = ping.header(0).parent().unwrap();

        buffer
    }
}

impl Encode<ServerTime, Vec<u8>> for Encoder {
    fn encode(&self, _data: ServerTime) -> Vec<u8> {
        let mut buffer = vec![0u8; (server_time_codec::SBE_BLOCK_LENGTH as usize) + 8];
        let mut server_time = ServerTimeEncoder::default();
        server_time = server_time.wrap(
            WriteBuf::new(buffer.as_mut_slice()),
            message_header_codec::ENCODED_LENGTH,
        );
        server_time = server_time.header(0).parent().unwrap();
        server_time.server_time(chrono::Utc::now().timestamp_nanos_opt().unwrap());

        buffer
    }
}

impl TryEncodeWithCtx<&TradeEvent, Vec<u8>> for Encoder {
    type Ctx = (u64, Symbol, chrono::DateTime<chrono::Utc>);

    fn try_encode(&self, data: &TradeEvent, ctx: Self::Ctx) -> Result<Vec<u8>> {
        let (trade_id, symbol, timestamp) = ctx;
        let mut buffer = vec![0u8; (trade_codec::SBE_BLOCK_LENGTH as usize) + 456];
        let mut trade = TradeEncoder::default();
        trade = trade.wrap(
            WriteBuf::new(buffer.as_mut_slice()),
            message_header_codec::ENCODED_LENGTH,
        );
        trade = trade.header(0).parent()?;
        trade.trade_id(trade_id);
        trade.symbol_id(u64::from(symbol));
        trade.price(data.price);
        trade.size(data.size);
        trade.taker_side(match data.side {
            matcher::Side::Bid => sbe::side::Side::Buy,
            matcher::Side::Ask => sbe::side::Side::Sell,
        });
        trade.buyer_order_id(data.buyer_order_id);
        trade.seller_order_id(data.seller_order_id);
        trade.time(timestamp.timestamp_nanos_opt().unwrap());

        Ok(buffer)
    }
}
