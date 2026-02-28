module.exports = {
    // Server Configuration
    port: process.env.PORT || 5000,
    nodeEnv: process.env.NODE_ENV || 'development',
    
    // JWT Configuration
    jwt: {
        secret: process.env.JWT_SECRET || 'your-secret-key-change-in-production',
        expiresIn: '24h',
        algorithm: 'HS256'
    },
    
    // CORS Configuration
    corsOrigin: process.env.CORS_ORIGIN || 'http://localhost:3000',
    
    // Rate Limiting
    rateLimit: {
        windowMs: 15 * 60 * 1000, // 15 minutes
        max: 100 // Max requests per window
    },
    
    // Security
    bcryptRounds: 10,
    
    // Data Configuration
    dataPath: process.env.DATA_PATH || './data/spine_records.json'
};
