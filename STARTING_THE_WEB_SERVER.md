# Starting the Web Server Manually

During demonstrations, you may need to start the web server and the frontend manually. Follow these step-by-step commands to get both running:

## 1. Start the Backend Server (Middleware)

The backend server is a Node.js application that runs on port `3001` by default.

1. Open a new terminal window.
2. Navigate to the `web/middleware` directory:
   ```powershell
   cd web/middleware
   ```
3. Start the server using Node:
   ```powershell
   node server.js
   ```

*Note: Ensure that no other process is using port `3001`. If it is, you might need to find the process ID (`netstat -ano | findstr :3001`) and kill it (`taskkill /PID <PID> /F`).*

## 2. Start the Frontend Server (Vite React App)

The frontend is a React application built with Vite, running on port `5173` by default.

1. Open another new terminal window (keep the backend server running).
2. Navigate to the `web/frontend` directory:
   ```powershell
   cd web/frontend
   ```
3. Start the Vite development server:
   ```powershell
   npm run dev
   ```

*Note: The frontend should now be accessible in your browser at `http://localhost:5173`. If port `5173` is occupied, Vite might pick the next available port.*

## Troubleshooting

- **Addresses already in use (EADDRINUSE):** This means previous instances of the server or frontend might still be running. You can kill them by finding their PIDs using `netstat` and terminating them using `taskkill` as shown in the notes above.
- **Missing Dependencies:** If you encounter any "module not found" errors, ensure you have run `npm install` in both the `web/middleware` and `web/frontend` directories.
