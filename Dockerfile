FROM gcc:latest

# Create app directory
WORKDIR /app

# Copy source code
COPY . .

# Build the server
RUN apt update && apt install -y cmake
RUN cmake . && make

# Expose the port (adjust if your server uses a different one)
EXPOSE 9000

# Start the server
CMD ["./server/a.out"]
