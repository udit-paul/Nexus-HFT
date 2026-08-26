import React, { useState, useEffect } from 'react';
import { io } from 'socket.io-client';
import './App.css';

const socket = io('http://localhost:3001');

function App() {
  const [bookData, setBookData] = useState({
    bids: [], asks: []
  });
  const [trades, setTrades] = useState([]);
  const [latency, setLatency] = useState(null);
  const [orderSide, setOrderSide] = useState('BUY');
  const [orderPrice, setOrderPrice] = useState('');
  const [orderQty, setOrderQty] = useState('');

  useEffect(() => {
    socket.on('book_update', (data) => {
      setBookData(data);
    });

    socket.on('trade', (data) => {
      setTrades(prev => [data, ...prev].slice(0, 10)); // Keep last 10 trades
    });
    
    socket.on('latency', (data) => {
      setLatency(data.nanoseconds);
    });

    socket.on('cleared', () => {
      setTrades([]);
      setLatency(null);
    });

    return () => {
      socket.off('book_update');
      socket.off('trade');
      socket.off('latency');
      socket.off('cleared');
    };
  }, []);

  const handleSubmit = (e) => {
    e.preventDefault();
    if (!orderPrice || !orderQty) return;
    
    socket.emit('submit_order', {
      side: orderSide,
      price: parseFloat(orderPrice),
      qty: parseInt(orderQty, 10)
    });
    
    setOrderPrice('');
    setOrderQty('');
  };

  const handleClear = () => {
    socket.emit('clear_book');
  };

  const getSpread = () => {
    if (bookData.asks.length > 0 && bookData.bids.length > 0) {
      return (bookData.asks[0][0] - bookData.bids[0][0]).toFixed(2);
    }
    return '-';
  };

  return (
    <div className="App">
      <header className="header">
        <div className="title-group">
          <h1>Nexus-HFT</h1>
          <span className="subtitle">Low-Latency Market Matching Engine</span>
        </div>
        <div className="status-container">
          {latency !== null && (
            <div className="latency">
              <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"><circle cx="12" cy="12" r="10"></circle><polyline points="12 6 12 12 16 14"></polyline></svg>
              {latency} ns
            </div>
          )}
          <button onClick={handleClear} className="clear-btn">Clear Book</button>
          <div className="status">
            <span className="dot connected"></span> ENGINE ACTIVE
          </div>
        </div>
      </header>
      
      <div className="main-content">
        <div className="panel order-book">
          <h2>Live Order Book</h2>
          <div className="book-container">
            <div className="asks">
              <div className="row header-row"><span>Ask Qty</span><span>Price</span></div>
              {bookData.asks.slice().reverse().map((ask, i) => (
                <div key={`ask-${i}`} className="row ask-row">
                  <span>{ask[1]}</span>
                  <span>{ask[0].toFixed(2)}</span>
                </div>
              ))}
              {bookData.asks.length === 0 && <div className="row ask-row"><span>-</span><span>-</span></div>}
            </div>
            <div className="spread">
              Spread: {getSpread()}
            </div>
            <div className="bids">
              <div className="row header-row"><span>Bid Qty</span><span>Price</span></div>
              {bookData.bids.map((bid, i) => (
                <div key={`bid-${i}`} className="row bid-row">
                  <span>{bid[1]}</span>
                  <span>{bid[0].toFixed(2)}</span>
                </div>
              ))}
              {bookData.bids.length === 0 && <div className="row bid-row"><span>-</span><span>-</span></div>}
            </div>
          </div>
        </div>

        <div className="panel entry-form">
          <h2>Submit Order</h2>
          <form onSubmit={handleSubmit}>
            <div className="form-group toggle">
              <button 
                type="button" 
                className={orderSide === 'BUY' ? 'active buy' : ''} 
                onClick={() => setOrderSide('BUY')}>BUY</button>
              <button 
                type="button" 
                className={orderSide === 'SELL' ? 'active sell' : ''} 
                onClick={() => setOrderSide('SELL')}>SELL</button>
            </div>
            <div className="form-group">
              <label>Price</label>
              <input type="number" step="0.01" value={orderPrice} onChange={e => setOrderPrice(e.target.value)} placeholder="e.g. 100.50" required />
            </div>
            <div className="form-group">
              <label>Quantity</label>
              <input type="number" step="1" min="1" value={orderQty} onChange={e => setOrderQty(e.target.value.replace(/[^0-9]/g, ''))} onKeyDown={(e) => { if (e.key === '.') e.preventDefault(); }} placeholder="e.g. 10" required />
            </div>
            <button type="submit" className={`submit-btn ${orderSide.toLowerCase()}`}>
              Submit {orderSide}
            </button>
          </form>
        </div>

        <div className="panel trades">
          <h2>Recent Trades</h2>
          <ul className="trade-list">
            {trades.length === 0 ? <li className="empty">No trades yet</li> : trades.map((t, i) => (
              <li key={i} className="trade-item">
                <div style={{display: 'flex', justifyContent: 'space-between', alignItems: 'center'}}>
                  <span className="trade-price">${t.price.toFixed(2)}</span>
                  <span className="trade-qty">x {t.qty}</span>
                </div>
                {t.latency && (
                  <div style={{fontSize: '11px', color: '#64748b', marginTop: '6px', display: 'flex', alignItems: 'center', gap: '4px'}}>
                    <svg xmlns="http://www.w3.org/2000/svg" width="10" height="10" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"><path d="M13 2L3 14h9l-1 8 10-12h-9l1-8z"></path></svg>
                    Matched in {t.latency} ns
                  </div>
                )}
              </li>
            ))}
          </ul>
        </div>
      </div>
    </div>
  );
}

export default App;
