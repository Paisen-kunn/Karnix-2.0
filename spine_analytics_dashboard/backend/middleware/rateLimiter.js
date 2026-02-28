// Rate Limiter Middleware
const rateLimit = require('express-rate-limit');

// General API rate limiter
const apiLimiter = rateLimit({
  windowMs: 15 * 60 * 1000, // 15 minutes
  max: 100, // Limit each IP to 100 requests per windowMs
  message: {
    success: false,
    message: 'Too many requests from this IP, please try again later'
  },
  standardHeaders: true, // Return rate limit info in the `RateLimit-*` headers
  legacyHeaders: false, // Disable the `X-RateLimit-*` headers
  handler: (req, res) => {
    res.status(429).json({
      success: false,
      message: 'Too many requests. Please try again later.',
      retryAfter: req.rateLimit.resetTime
    });
  }
});

// Strict rate limiter for authentication endpoints
const authLimiter = rateLimit({
  windowMs: 15 * 60 * 1000, // 15 minutes
  max: 5, // Limit each IP to 5 login requests per windowMs
  skipSuccessfulRequests: true, // Don't count successful requests
  message: {
    success: false,
    message: 'Too many login attempts, please try again later'
  },
  handler: (req, res) => {
    console.warn(`Rate limit exceeded for IP: ${req.ip}, Path: ${req.path}`);
    res.status(429).json({
      success: false,
      message: 'Too many authentication attempts. Please try again in 15 minutes.',
      retryAfter: req.rateLimit.resetTime
    });
  }
});

// Create/update operations limiter
const createLimiter = rateLimit({
  windowMs: 60 * 1000, // 1 minute
  max: 10, // Limit each IP to 10 create operations per minute
  message: {
    success: false,
    message: 'Too many create/update requests, please slow down'
  },
  handler: (req, res) => {
    res.status(429).json({
      success: false,
      message: 'Too many requests. Please wait a moment before trying again.',
      retryAfter: req.rateLimit.resetTime
    });
  }
});

// Search/query operations limiter
const searchLimiter = rateLimit({
  windowMs: 60 * 1000, // 1 minute
  max: 30, // Limit each IP to 30 searches per minute
  message: {
    success: false,
    message: 'Too many search requests, please slow down'
  }
});

// Export/download limiter
const exportLimiter = rateLimit({
  windowMs: 60 * 60 * 1000, // 1 hour
  max: 10, // Limit each IP to 10 exports per hour
  message: {
    success: false,
    message: 'Too many export requests, please try again later'
  },
  handler: (req, res) => {
    res.status(429).json({
      success: false,
      message: 'Export limit reached. Please try again in an hour.',
      retryAfter: req.rateLimit.resetTime
    });
  }
});

// Analytics endpoints limiter
const analyticsLimiter = rateLimit({
  windowMs: 60 * 1000, // 1 minute
  max: 20, // Limit each IP to 20 analytics requests per minute
  message: {
    success: false,
    message: 'Too many analytics requests, please slow down'
  }
});

// Lenient limiter for general read operations
const readLimiter = rateLimit({
  windowMs: 60 * 1000, // 1 minute
  max: 200, // Limit each IP to 200 read operations per minute (increased for development)
  message: {
    success: false,
    message: 'Too many requests, please slow down'
  }
});

// Custom rate limit skip function
const skipRateLimitForTrustedIPs = (req) => {
  // Add trusted IPs here (e.g., localhost for development)
  const trustedIPs = ['127.0.0.1', '::1', 'localhost'];
  return trustedIPs.includes(req.ip);
};

// Configuration object for easy customization
const rateLimitConfig = {
  api: {
    windowMs: 15 * 60 * 1000,
    max: 100
  },
  auth: {
    windowMs: 15 * 60 * 1000,
    max: 5
  },
  create: {
    windowMs: 60 * 1000,
    max: 10
  },
  search: {
    windowMs: 60 * 1000,
    max: 30
  },
  export: {
    windowMs: 60 * 60 * 1000,
    max: 10
  },
  analytics: {
    windowMs: 60 * 1000,
    max: 20
  },
  read: {
    windowMs: 60 * 1000,
    max: 50
  }
};

module.exports = {
  apiLimiter,
  authLimiter,
  createLimiter,
  searchLimiter,
  exportLimiter,
  analyticsLimiter,
  readLimiter,
  rateLimitConfig,
  skipRateLimitForTrustedIPs
};
