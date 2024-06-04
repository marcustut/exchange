use types::{Message, Trade};

use super::*;

pub trait Decode<In> {
    type Out;

    fn decode(&self, data: In) -> Self::Out;
}

pub trait TryDecode<In, Out> {
    fn try_decode(&self, data: In) -> Result<Out>;
}

pub trait TryDecodeWithCtx<In> {
    type Out;
    type Ctx;

    fn try_decode(&self, data: In, ctx: Self::Ctx) -> Result<Self::Out>;
}

#[derive(Debug)]
pub struct Decoder {}

impl Decode<(u64, i16, i8)> for Decoder {
    type Out = f64;

    fn decode(&self, data: (u64, i16, i8)) -> Self::Out {
        let (mantissa, exponent, sign) = data;
        sign as f64 * mantissa as f64 * 2.0f64.powi(exponent.into())
    }
}

impl TryDecode<&[u8], Trade> for Decoder {
    fn try_decode(&self, data: &[u8]) -> Result<Trade> {
        let message_header_decoder = MessageHeaderDecoder::default().wrap(ReadBuf::new(data), 0);

        if message_header_decoder.template_id() != trade_codec::SBE_TEMPLATE_ID {
            return Err(Error::SbeInvalidTemplateId(
                message_header_decoder.template_id(),
            ));
        }

        let trade_decoder = TradeDecoder::default().header(message_header_decoder);

        Ok(Trade {
            trade_id: trade_decoder.trade_id(),
            symbol_id: trade_decoder.symbol_id(),
            price: trade_decoder.price(),
            size: trade_decoder.size(),
            taker_side: match trade_decoder.taker_side() {
                sbe::side::Side::Buy => matcher::Side::Bid,
                sbe::side::Side::Sell => matcher::Side::Ask,
                sbe::side::Side::NullVal => unreachable!(),
            },
            buy_order_id: trade_decoder.buyer_order_id(),
            sell_order_id: trade_decoder.seller_order_id(),
            timestamp: trade_decoder.time(),
        })
    }
}

impl TryDecode<&[u8], Message> for Decoder {
    fn try_decode(&self, data: &[u8]) -> Result<Message> {
        let message_header_decoder = MessageHeaderDecoder::default().wrap(ReadBuf::new(data), 0);

        match message_header_decoder.template_id() {
            trade_codec::SBE_TEMPLATE_ID => Ok(Message::Trade(self.try_decode(data)?)),
            _ => Err(Error::SbeInvalidTemplateId(
                message_header_decoder.template_id(),
            )),
        }
    }
}
