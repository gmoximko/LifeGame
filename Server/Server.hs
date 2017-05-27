{-# LANGUAGE OverloadedStrings #-}

import System.Environment (getArgs)

import qualified Data.Map as Map (Map(..), toList, empty, insert)

import Network (withSocketsDo)
import Network.Socket
import Network.URI (uriPath)
import Network.HTTP (receiveHTTP, respondHTTP, defaultUserAgent, mkHeader, Request(..), Response(..), HeaderName(..), Header(..))
import qualified Network.TCP as TCP (openSocketStream, close, HandleStream(..))

import Control.Concurrent (forkIO)
import Control.Monad (forever)

main :: IO ()
main = withSocketsDo $ do 
    args <- getArgs
    let pORT = getPort args
    let anyAddr = SockAddrInet pORT iNADDR_ANY
    sock <- socket AF_INET Stream defaultProtocol
    setSocketOption sock ReuseAddr 1
    bind sock anyAddr
    listen sock sOMAXCONN

    addr <- getSocketName sock
    (host, service) <- getNameInfo [] True True addr
    print $ (\x y -> x ++ ":" ++ y) <$> host <*> service
    
    forever $ sockLoop sock
    close sock

sockLoop :: Socket -> IO ()
sockLoop sock = do 
    (newSock, remoteAddr) <- accept sock
    (host, service) <- getNameInfo [] True True remoteAddr
    processHTTP newSock $ getHostPort host service
    
processHTTP :: Socket -> Maybe (String, Int) -> IO ()
processHTTP sock Nothing = (print "Invalid address!") >> (close sock)
processHTTP sock (Just (host, port)) = do
    forkIO $ do
        stream <- TCP.openSocketStream host port sock :: IO (TCP.HandleStream String)
        request <- receiveHTTP stream 
        case request of 
            (Left err) -> print err
            (Right rq) -> processHTTPRequest rq stream
        TCP.close stream
    return ()

processHTTPRequest :: Request String -> TCP.HandleStream String -> IO ()
processHTTPRequest rq stream = (print rq) >> (respondHTTP stream $ responseGET $ (uriPath . rqURI) rq)

responseGET :: String -> Response String
responseGET path
    | path == "/games" = Response 
        { rspCode = (2, 0, 0)
        , rspReason = "OK"
        , rspHeaders = headers
        , rspBody = foldl (\body (addr, params) -> concat [body, addr, " ", params, "\n"]) "" [] -- $ Map.toList
        }
    | otherwise = badResponse

badResponse :: Response String
badResponse = Response { rspCode = (4, 0, 0)
                       , rspReason = "Bad Request"
                       , rspHeaders = headers
                       , rspBody = [] 
                       }

headers :: [Header]
headers = [mkHeader HdrUserAgent defaultUserAgent]

getPort :: [String] -> PortNumber
getPort [] = aNY_PORT
getPort (x:_) = fromInteger $ (read x :: Integer)

getHostPort :: Maybe String -> Maybe String -> Maybe (String, Int)
getHostPort (Just host) (Just service) = Just (host, read service)
getHostPort _ _ = Nothing
